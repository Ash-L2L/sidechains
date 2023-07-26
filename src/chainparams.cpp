// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <assert.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <key.h>
#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>


static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.hashWithdrawalBundle.SetNull(); // This could be some useful hash
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 */
static CBlock CreateGenesisBlock(uint32_t nTime, int32_t nVersion, const CAmount& genesisReward)
{
    // Note: For sidechains the timestamp should be:
    // "mainchainBlockHeight:mainchainBlockHash"
    const char* pszTimestamp = "nnnnnn:0xnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;

        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x359d17fc7cc60653fb72bbec271efab88af16ba9f15a55b060fe632c7de5e978");
        // FIXME: replace with a real key hash
        consensus.IcannRegistrationKeyHash =
            uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x1a;
        pchMessageStart[1] = 0xc2;
        pchMessageStart[2] = 0x18;
        pchMessageStart[3] = 0x3d;
        nDefaultPort = 2600;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1677671991, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        // FIXME: remove
        std::printf("Genesis Hash: 0x%s\n", consensus.hashGenesisBlock.ToString().c_str());
        std::fprintf(stderr, "Genesis Hash: 0x%s\n", consensus.hashGenesisBlock.ToString().c_str());
        std::printf("Genesis Merkle Root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());
        std::fprintf(stderr, "Genesis Merkle Root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());

        assert(consensus.hashGenesisBlock == uint256S("0xc4229719a13b1bd38c9c157d534e101badf74702736456be75d8f8808a9b1aed"));
        assert(genesis.hashMerkleRoot == uint256S("0x8eb1364f43885edf1322b2d32095e57abb03c32a61a80ac25c8db3de58e16b8a"));

        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,125);
        base58Prefixes[MAINCHAIN_PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[MAINCHAIN_REGTEST_PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "sc";

        // TODO more seed nodes
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                { 0, uint256S("0x359d17fc7cc60653fb72bbec271efab88af16ba9f15a55b060fe632c7de5e978")},
            }
        };

        // TODO update these once your sidechain has been active for a while
        chainTxData = ChainTxData{
            // Data as of block 000000000000000000d97e53664d17967bd4ee50b23abb92e54a34eb222d15ae (height 478913).
            0, // * UNIX timestamp of last known number of transactions
            0, // * total number of transactions between genesis and that timestamp
               //   (the tx=... number in the SetBestChain debug.log lines)
            0  // * estimated number of transactions per second after that timestamp
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Height = 0; // always enforce P2SH BIP16 on regtest
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for bitnames
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x6fb2c5081b13ecf369b324db61406244c7df519b4676e28b536a4ef546e387eb");
        // FIXME: replace with a real key hash
        std::string icann_registration_passphrase =
            "layer two labs icann reg test";
        uint256 icann_reg_secret = uint256();
        CHash256().Write((unsigned char*) icann_registration_passphrase.data(),
                         icann_registration_passphrase.size())
                  .Finalize((unsigned char*) &icann_reg_secret);
        CKey icann_reg_key = CKey();
        icann_reg_key.Set(
            icann_reg_secret.begin(),
            icann_reg_secret.end(),
            true
        );
        CPubKey icann_reg_pubkey = icann_reg_key.GetPubKey();
        consensus.IcannRegistrationKeyHash = icann_reg_pubkey.GetHash();

        pchMessageStart[0] = 0x36;
        pchMessageStart[1] = 0x4d;
        pchMessageStart[2] = 0x1c;
        pchMessageStart[3] = 0x0c;
        nDefaultPort = 12744;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1677672170, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        // FIXME: remove
        std::printf("Genesis Hash: 0x%s\n", consensus.hashGenesisBlock.ToString().c_str());
        std::fprintf(stderr, "Genesis Hash: 0x%s\n", consensus.hashGenesisBlock.ToString().c_str());
        std::printf("Genesis Merkle Root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());
        std::fprintf(stderr, "Genesis Merkle Root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());


        assert(consensus.hashGenesisBlock == uint256S("0x3b184bdff1320d46b30b6366e881636ef8ebd35647f4192c9cdc6b796651af5b"));
        assert(genesis.hashMerkleRoot == uint256S("0xdd67edb32e51fe80f60b0bee6b54eee556bda41df00bcef289044aefaafcaadd"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
            }
        };

        // Doesn't need to be updated
        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[MAINCHAIN_PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[MAINCHAIN_REGTEST_PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,125);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "scrt";
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
