// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>

#include <consensus/consensus.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <consensus/validation.h>

#include <openssl/x509v3.h>
#include <secp256k1.h>

// TODO remove the following dependencies
#include <chain.h>
#include <coins.h>
#include <utilmoneystr.h>

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2
                      && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    for (const auto& txout : tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, &tx.vin[i].scriptWitness, flags);
    }
    return nSigOps;
}

/* Check that an X509 certificate is valid, IGNORING the validity period.
   The validity period should be checked during block validation.
   Does NOT validate against a particular domain name
   Does NOT check for revocation
   Does NOT check CT */
bool validateCertificate(X509* cert, X509_STORE* store) {
    // use the first valid time as the timestamp
    const ASN1_TIME* not_before_asn1 = X509_get0_notBefore(cert);
    tm not_before_tm;
    if (ASN1_TIME_to_tm(not_before_asn1, &not_before_tm) != 1) {
        return false;
    }
    // one second after the minimum time
    time_t validation_time = mktime(&not_before_tm);
    if (validation_time < std::numeric_limits<time_t>::max()) {
        validation_time++;
    } else {
        return false;
    }
    
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
    if (!ctx) {
        // Handle context creation error
        return false;
    }

    // set check time flag so that we can ignore the validity period
    X509_STORE_set_flags(
        store,
        X509_V_FLAG_X509_STRICT & X509_V_FLAG_USE_CHECK_TIME
    );
    if (X509_STORE_CTX_init(ctx, store, cert, nullptr) != 1) {
        // Handle context initialization error
        X509_STORE_CTX_free(ctx);
        return false;
    }
    X509_STORE_CTX_set_time(ctx, 0, validation_time);

    int result = X509_verify_cert(ctx);
    X509_STORE_CTX_free(ctx);

    if (result == 1) {
        // Certificate is valid
        return true;
    } else {
        // Certificate validation failed
        return false;
    }
}

/* Check that a certificate applies to the specified domain name.
 * Does NOT check that the certificate itself is valid.
 * The domain name MUST be provided in 'Preferred name syntax' (RFC 1034 §3.5).
 * International domain names MUST be given in A-label form. */
bool validateCertificateIncludesDomain(X509* cert, std::string domain_name) {
    const unsigned int FLAGS = X509_CHECK_FLAG_SINGLE_LABEL_SUBDOMAINS;
    int result = X509_check_host(
        cert,
        domain_name.data(),
        domain_name.length(),
        FLAGS,
        nullptr
    );
    if (result == 1) {
        return true;
    } else {
        return false;
    }
}


bool CheckTransaction(const CTransaction& tx, CValidationState &state, bool fCheckDuplicateInputs)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT)
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");


    bool fCreateBitName = tx.nVersion == TRANSACTION_BITNAME_CREATE_VERSION;
    bool fUpdateBitName = tx.nVersion == TRANSACTION_BITNAME_UPDATE_VERSION;
    bool fBitName = fCreateBitName || fUpdateBitName;
    bool fBitNameRegistration =
        fCreateBitName && (tx.name_hash != uint256());

    // Create BitName transactions must have at least 1 output
    if (fCreateBitName && tx.vout.size() < 1)
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-create-bitname-vout-size");
    // ICANN BitName registrations must have a valid signature over the name hash and first output
    if (fBitNameRegistration && tx.fIcann) {
        uint256 registration_output_hash = SerializeHash(tx.vout[1]);
        CSHA256 hasher = CSHA256();
        uint256 signable_registration_hash = uint256();
        hasher.Write(tx.name_hash.begin(), tx.name_hash.size());
        hasher.Write(
            registration_output_hash.begin(),
            registration_output_hash.size()
        );
        hasher.Finalize(signable_registration_hash.begin());

        // see if we can recover a pubkey from the signature
        CPubKey pubkey = CPubKey();
        std::vector<uint8_t> vchSig =
            std::vector<uint8_t>(tx.icann_sig.begin(), tx.icann_sig.end());
        if (!pubkey.RecoverCompact(signable_registration_hash, vchSig)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
        if (!pubkey.IsFullyValid()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
        secp256k1_context* secp256k1_ctxt = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
        secp256k1_ecdsa_signature ecdsa_sig;
        if (secp256k1_ecdsa_signature_parse_compact(secp256k1_ctxt, &ecdsa_sig, tx.icann_sig.data()) != 1) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
        std::vector<uint8_t> der_sig =
            std::vector<uint8_t>(CPubKey::SIGNATURE_SIZE, 0);
        size_t der_sig_size = CPubKey::SIGNATURE_SIZE;
        if (secp256k1_ecdsa_signature_serialize_der(secp256k1_ctxt, der_sig.data(), &der_sig_size, &ecdsa_sig) != 1) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
        if (!pubkey.Verify(signable_registration_hash, der_sig)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
    }

    // Update BitName transactions must have at least 1 output
    if (fUpdateBitName && tx.vout.size() < 1)
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-update-bitname-vout-size");
    
    // Update BitName transactions must update at least 1 field
    if (fUpdateBitName && !(tx.fCommitment || tx.fIn4 || tx.cpk))
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-update-bitname-no-updates");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    std::vector<CTxOut>::const_iterator it;
    it = tx.vout.begin();
    if (fBitName) {
        // The first output from a create/update bitname tx must have a value of 1
        if (it->nValue > 1)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        else if (it->nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        it++;
    }
    for (; it != tx.vout.end(); it++)
    {
        if (it->nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (it->nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += it->nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs - note that this check is slow so we skip it in CheckBlock
    if (fCheckDuplicateInputs) {
        std::set<COutPoint> vInOutPoints;
        for (const auto& txin : tx.vin)
        {
            if (!vInOutPoints.insert(txin.prevout).second)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        }
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missingorspent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    bool fCreateBitName = tx.nVersion == TRANSACTION_BITNAME_CREATE_VERSION;
    bool fUpdateBitName = tx.nVersion == TRANSACTION_BITNAME_UPDATE_VERSION;

    // in a create bitname tx, if the 'name_hash' field is set (registration),
    // then there must exist a bitname reservation input at the last index of
    // the inputs, for which the reserved commitment must be equal to the hash
    // of txinputshash+commitment+name_hash, where the txinputshash is tha hash
    // of tx inputs for the tx that created the bitname reservation.
    if (fCreateBitName) {
        if (tx.name_hash != uint256()) {
            const COutPoint& lastOutpoint = tx.vin.back().prevout;
            const Coin& lastInputCoin = inputs.AccessCoin(lastOutpoint);
            // last input coin must be a reservation
            if (!lastInputCoin.fBitNameReservation)
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-missing-reservation");
            uint256 commitment = lastInputCoin.commitment;
            uint256 name_hash = tx.name_hash;
            uint256 sok = tx.sok; // statement of knowledge, aka salt
            // compute the hash of name_hash + sok
            uint256 hash_result;
            CHash256().Write(name_hash.begin(), name_hash.size())
                      .Write(sok.begin(), sok.size())
                      .Finalize((unsigned char*) &hash_result);
            if (commitment != hash_result)
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-wrong-commitment");
        }
    }

    // in an update bitname tx, there must exist a bitname input
    // at the last index of the inputs
    if (fUpdateBitName) {
        const COutPoint& lastOutpoint = tx.vin.back().prevout;
        const Coin& lastInputCoin = inputs.AccessCoin(lastOutpoint);
        // last input coin must be a bitname
        if (!lastInputCoin.fBitName)
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-missing-bitname");
    }

    uint256 nAssetIDFound = uint256();
    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // If prev is coinbase, check that it's matured
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < COINBASE_MATURITY) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }

        if ((coin.nAssetID != uint256()) && (nAssetIDFound != uint256()) && coin.nAssetID != nAssetIDFound)
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-mixed-assets");

        nAssetIDFound = coin.nAssetID;

    }

    const CAmount value_out = tx.GetValueOut();
    if (nValueIn < value_out) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));
    }

    // Tally transaction fees
    const CAmount txfee_aux = nValueIn - value_out;
    if (!MoneyRange(txfee_aux)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-outofrange");
    }

    txfee = txfee_aux;
    return true;
}
