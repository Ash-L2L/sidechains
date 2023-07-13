// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>

#include <consensus/consensus.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <consensus/validation.h>

#include <string>
#include <boost/algorithm/string.hpp>
#include <openssl/x509v3.h>
#include <regex>
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

bool is_icann_tld(std::string tld) {
    // version 2023070600
    /*
    curl --silent https://data.iana.org/TLD/tlds-alpha-by-domain.txt \
        | tr '[:upper:]' '[:lower:]' \
        | tail -n +2 \
        | awk '{printf "\"%s\", ", $0} END {print ""}' \
        | fmt -w 72
    */
    std::set<std::string> tlds = {
        "aaa", "aarp", "abb", "abbott", "abbvie", "abc", "able", "abogado",
        "abudhabi", "ac", "academy", "accenture", "accountant", "accountants",
        "aco", "actor", "ad", "ads", "adult", "ae", "aeg", "aero", "aetna",
        "af", "afl", "africa", "ag", "agakhan", "agency", "ai", "aig",
        "airbus", "airforce", "airtel", "akdn", "al", "alibaba", "alipay",
        "allfinanz", "allstate", "ally", "alsace", "alstom", "am", "amazon",
        "americanexpress", "americanfamily", "amex", "amfam", "amica",
        "amsterdam", "analytics", "android", "anquan", "anz", "ao", "aol",
        "apartments", "app", "apple", "aq", "aquarelle", "ar", "arab",
        "aramco", "archi", "army", "arpa", "art", "arte", "as", "asda", "asia",
        "associates", "at", "athleta", "attorney", "au", "auction", "audi",
        "audible", "audio", "auspost", "author", "auto", "autos", "avianca",
        "aw", "aws", "ax", "axa", "az", "azure", "ba", "baby", "baidu",
        "banamex", "bananarepublic", "band", "bank", "bar", "barcelona",
        "barclaycard", "barclays", "barefoot", "bargains", "baseball",
        "basketball", "bauhaus", "bayern", "bb", "bbc", "bbt", "bbva",
        "bcg", "bcn", "bd", "be", "beats", "beauty", "beer", "bentley",
        "berlin", "best", "bestbuy", "bet", "bf", "bg", "bh", "bharti",
        "bi", "bible", "bid", "bike", "bing", "bingo", "bio", "biz", "bj",
        "black", "blackfriday", "blockbuster", "blog", "bloomberg", "blue",
        "bm", "bms", "bmw", "bn", "bnpparibas", "bo", "boats", "boehringer",
        "bofa", "bom", "bond", "boo", "book", "booking", "bosch", "bostik",
        "boston", "bot", "boutique", "box", "br", "bradesco", "bridgestone",
        "broadway", "broker", "brother", "brussels", "bs", "bt", "build",
        "builders", "business", "buy", "buzz", "bv", "bw", "by", "bz", "bzh",
        "ca", "cab", "cafe", "cal", "call", "calvinklein", "cam", "camera",
        "camp", "canon", "capetown", "capital", "capitalone", "car", "caravan",
        "cards", "care", "career", "careers", "cars", "casa", "case", "cash",
        "casino", "cat", "catering", "catholic", "cba", "cbn", "cbre", "cbs",
        "cc", "cd", "center", "ceo", "cern", "cf", "cfa", "cfd", "cg", "ch",
        "chanel", "channel", "charity", "chase", "chat", "cheap", "chintai",
        "christmas", "chrome", "church", "ci", "cipriani", "circle", "cisco",
        "citadel", "citi", "citic", "city", "cityeats", "ck", "cl", "claims",
        "cleaning", "click", "clinic", "clinique", "clothing", "cloud",
        "club", "clubmed", "cm", "cn", "co", "coach", "codes", "coffee",
        "college", "cologne", "com", "comcast", "commbank", "community",
        "company", "compare", "computer", "comsec", "condos", "construction",
        "consulting", "contact", "contractors", "cooking", "cool", "coop",
        "corsica", "country", "coupon", "coupons", "courses", "cpa", "cr",
        "credit", "creditcard", "creditunion", "cricket", "crown", "crs",
        "cruise", "cruises", "cu", "cuisinella", "cv", "cw", "cx", "cy",
        "cymru", "cyou", "cz", "dabur", "dad", "dance", "data", "date",
        "dating", "datsun", "day", "dclk", "dds", "de", "deal", "dealer",
        "deals", "degree", "delivery", "dell", "deloitte", "delta", "democrat",
        "dental", "dentist", "desi", "design", "dev", "dhl", "diamonds",
        "diet", "digital", "direct", "directory", "discount", "discover",
        "dish", "diy", "dj", "dk", "dm", "dnp", "do", "docs", "doctor", "dog",
        "domains", "dot", "download", "drive", "dtv", "dubai", "dunlop",
        "dupont", "durban", "dvag", "dvr", "dz", "earth", "eat", "ec", "eco",
        "edeka", "edu", "education", "ee", "eg", "email", "emerck", "energy",
        "engineer", "engineering", "enterprises", "epson", "equipment", "er",
        "ericsson", "erni", "es", "esq", "estate", "et", "etisalat", "eu",
        "eurovision", "eus", "events", "exchange", "expert", "exposed",
        "express", "extraspace", "fage", "fail", "fairwinds", "faith",
        "family", "fan", "fans", "farm", "farmers", "fashion", "fast", "fedex",
        "feedback", "ferrari", "ferrero", "fi", "fidelity", "fido", "film",
        "final", "finance", "financial", "fire", "firestone", "firmdale",
        "fish", "fishing", "fit", "fitness", "fj", "fk", "flickr", "flights",
        "flir", "florist", "flowers", "fly", "fm", "fo", "foo", "food",
        "football", "ford", "forex", "forsale", "forum", "foundation", "fox",
        "fr", "free", "fresenius", "frl", "frogans", "frontdoor", "frontier",
        "ftr", "fujitsu", "fun", "fund", "furniture", "futbol", "fyi", "ga",
        "gal", "gallery", "gallo", "gallup", "game", "games", "gap", "garden",
        "gay", "gb", "gbiz", "gd", "gdn", "ge", "gea", "gent", "genting",
        "george", "gf", "gg", "ggee", "gh", "gi", "gift", "gifts", "gives",
        "giving", "gl", "glass", "gle", "global", "globo", "gm", "gmail",
        "gmbh", "gmo", "gmx", "gn", "godaddy", "gold", "goldpoint", "golf",
        "goo", "goodyear", "goog", "google", "gop", "got", "gov", "gp", "gq",
        "gr", "grainger", "graphics", "gratis", "green", "gripe", "grocery",
        "group", "gs", "gt", "gu", "guardian", "gucci", "guge", "guide",
        "guitars", "guru", "gw", "gy", "hair", "hamburg", "hangout", "haus",
        "hbo", "hdfc", "hdfcbank", "health", "healthcare", "help", "helsinki",
        "here", "hermes", "hiphop", "hisamitsu", "hitachi", "hiv", "hk",
        "hkt", "hm", "hn", "hockey", "holdings", "holiday", "homedepot",
        "homegoods", "homes", "homesense", "honda", "horse", "hospital",
        "host", "hosting", "hot", "hoteles", "hotels", "hotmail", "house",
        "how", "hr", "hsbc", "ht", "hu", "hughes", "hyatt", "hyundai", "ibm",
        "icbc", "ice", "icu", "id", "ie", "ieee", "ifm", "ikano", "il", "im",
        "imamat", "imdb", "immo", "immobilien", "in", "inc", "industries",
        "infiniti", "info", "ing", "ink", "institute", "insurance", "insure",
        "int", "international", "intuit", "investments", "io", "ipiranga",
        "iq", "ir", "irish", "is", "ismaili", "ist", "istanbul", "it", "itau",
        "itv", "jaguar", "java", "jcb", "je", "jeep", "jetzt", "jewelry",
        "jio", "jll", "jm", "jmp", "jnj", "jo", "jobs", "joburg", "jot",
        "joy", "jp", "jpmorgan", "jprs", "juegos", "juniper", "kaufen",
        "kddi", "ke", "kerryhotels", "kerrylogistics", "kerryproperties",
        "kfh", "kg", "kh", "ki", "kia", "kids", "kim", "kinder", "kindle",
        "kitchen", "kiwi", "km", "kn", "koeln", "komatsu", "kosher", "kp",
        "kpmg", "kpn", "kr", "krd", "kred", "kuokgroup", "kw", "ky", "kyoto",
        "kz", "la", "lacaixa", "lamborghini", "lamer", "lancaster", "land",
        "landrover", "lanxess", "lasalle", "lat", "latino", "latrobe", "law",
        "lawyer", "lb", "lc", "lds", "lease", "leclerc", "lefrak", "legal",
        "lego", "lexus", "lgbt", "li", "lidl", "life", "lifeinsurance",
        "lifestyle", "lighting", "like", "lilly", "limited", "limo", "lincoln",
        "link", "lipsy", "live", "living", "lk", "llc", "llp", "loan", "loans",
        "locker", "locus", "lol", "london", "lotte", "lotto", "love", "lpl",
        "lplfinancial", "lr", "ls", "lt", "ltd", "ltda", "lu", "lundbeck",
        "luxe", "luxury", "lv", "ly", "ma", "madrid", "maif", "maison",
        "makeup", "man", "management", "mango", "map", "market", "marketing",
        "markets", "marriott", "marshalls", "mattel", "mba", "mc", "mckinsey",
        "md", "me", "med", "media", "meet", "melbourne", "meme", "memorial",
        "men", "menu", "merckmsd", "mg", "mh", "miami", "microsoft", "mil",
        "mini", "mint", "mit", "mitsubishi", "mk", "ml", "mlb", "mls",
        "mm", "mma", "mn", "mo", "mobi", "mobile", "moda", "moe", "moi",
        "mom", "monash", "money", "monster", "mormon", "mortgage", "moscow",
        "moto", "motorcycles", "mov", "movie", "mp", "mq", "mr", "ms", "msd",
        "mt", "mtn", "mtr", "mu", "museum", "music", "mutual", "mv", "mw",
        "mx", "my", "mz", "na", "nab", "nagoya", "name", "natura", "navy",
        "nba", "nc", "ne", "nec", "net", "netbank", "netflix", "network",
        "neustar", "new", "news", "next", "nextdirect", "nexus", "nf", "nfl",
        "ng", "ngo", "nhk", "ni", "nico", "nike", "nikon", "ninja", "nissan",
        "nissay", "nl", "no", "nokia", "northwesternmutual", "norton", "now",
        "nowruz", "nowtv", "np", "nr", "nra", "nrw", "ntt", "nu", "nyc", "nz",
        "obi", "observer", "office", "okinawa", "olayan", "olayangroup",
        "oldnavy", "ollo", "om", "omega", "one", "ong", "onl", "online",
        "ooo", "open", "oracle", "orange", "org", "organic", "origins",
        "osaka", "otsuka", "ott", "ovh", "pa", "page", "panasonic", "paris",
        "pars", "partners", "parts", "party", "passagens", "pay", "pccw",
        "pe", "pet", "pf", "pfizer", "pg", "ph", "pharmacy", "phd", "philips",
        "phone", "photo", "photography", "photos", "physio", "pics", "pictet",
        "pictures", "pid", "pin", "ping", "pink", "pioneer", "pizza", "pk",
        "pl", "place", "play", "playstation", "plumbing", "plus", "pm", "pn",
        "pnc", "pohl", "poker", "politie", "porn", "post", "pr", "pramerica",
        "praxi", "press", "prime", "pro", "prod", "productions", "prof",
        "progressive", "promo", "properties", "property", "protection", "pru",
        "prudential", "ps", "pt", "pub", "pw", "pwc", "py", "qa", "qpon",
        "quebec", "quest", "racing", "radio", "re", "read", "realestate",
        "realtor", "realty", "recipes", "red", "redstone", "redumbrella",
        "rehab", "reise", "reisen", "reit", "reliance", "ren", "rent",
        "rentals", "repair", "report", "republican", "rest", "restaurant",
        "review", "reviews", "rexroth", "rich", "richardli", "ricoh", "ril",
        "rio", "rip", "ro", "rocher", "rocks", "rodeo", "rogers", "room",
        "rs", "rsvp", "ru", "rugby", "ruhr", "run", "rw", "rwe", "ryukyu",
        "sa", "saarland", "safe", "safety", "sakura", "sale", "salon",
        "samsclub", "samsung", "sandvik", "sandvikcoromant", "sanofi", "sap",
        "sarl", "sas", "save", "saxo", "sb", "sbi", "sbs", "sc", "sca", "scb",
        "schaeffler", "schmidt", "scholarships", "school", "schule", "schwarz",
        "science", "scot", "sd", "se", "search", "seat", "secure", "security",
        "seek", "select", "sener", "services", "seven", "sew", "sex", "sexy",
        "sfr", "sg", "sh", "shangrila", "sharp", "shaw", "shell", "shia",
        "shiksha", "shoes", "shop", "shopping", "shouji", "show", "showtime",
        "si", "silk", "sina", "singles", "site", "sj", "sk", "ski", "skin",
        "sky", "skype", "sl", "sling", "sm", "smart", "smile", "sn", "sncf",
        "so", "soccer", "social", "softbank", "software", "sohu", "solar",
        "solutions", "song", "sony", "soy", "spa", "space", "sport", "spot",
        "sr", "srl", "ss", "st", "stada", "staples", "star", "statebank",
        "statefarm", "stc", "stcgroup", "stockholm", "storage", "store",
        "stream", "studio", "study", "style", "su", "sucks", "supplies",
        "supply", "support", "surf", "surgery", "suzuki", "sv", "swatch",
        "swiss", "sx", "sy", "sydney", "systems", "sz", "tab", "taipei",
        "talk", "taobao", "target", "tatamotors", "tatar", "tattoo", "tax",
        "taxi", "tc", "tci", "td", "tdk", "team", "tech", "technology", "tel",
        "temasek", "tennis", "teva", "tf", "tg", "th", "thd", "theater",
        "theatre", "tiaa", "tickets", "tienda", "tiffany", "tips", "tires",
        "tirol", "tj", "tjmaxx", "tjx", "tk", "tkmaxx", "tl", "tm", "tmall",
        "tn", "to", "today", "tokyo", "tools", "top", "toray", "toshiba",
        "total", "tours", "town", "toyota", "toys", "tr", "trade", "trading",
        "training", "travel", "travelers", "travelersinsurance", "trust",
        "trv", "tt", "tube", "tui", "tunes", "tushu", "tv", "tvs", "tw", "tz",
        "ua", "ubank", "ubs", "ug", "uk", "unicom", "university", "uno", "uol",
        "ups", "us", "uy", "uz", "va", "vacations", "vana", "vanguard", "vc",
        "ve", "vegas", "ventures", "verisign", "versicherung", "vet", "vg",
        "vi", "viajes", "video", "vig", "viking", "villas", "vin", "vip",
        "virgin", "visa", "vision", "viva", "vivo", "vlaanderen", "vn",
        "vodka", "volkswagen", "volvo", "vote", "voting", "voto", "voyage",
        "vu", "vuelos", "wales", "walmart", "walter", "wang", "wanggou",
        "watch", "watches", "weather", "weatherchannel", "webcam",
        "weber", "website", "wed", "wedding", "weibo", "weir", "wf",
        "whoswho", "wien", "wiki", "williamhill", "win", "windows", "wine",
        "winners", "wme", "wolterskluwer", "woodside", "work", "works",
        "world", "wow", "ws", "wtc", "wtf", "xbox", "xerox", "xfinity",
        "xihuan", "xin", "xn--11b4c3d", "xn--1ck2e1b", "xn--1qqw23a",
        "xn--2scrj9c", "xn--30rr7y", "xn--3bst00m", "xn--3ds443g",
        "xn--3e0b707e", "xn--3hcrj9c", "xn--3pxu8k", "xn--42c2d9a",
        "xn--45br5cyl", "xn--45brj9c", "xn--45q11c", "xn--4dbrk0ce",
        "xn--4gbrim", "xn--54b7fta0cc", "xn--55qw42g", "xn--55qx5d",
        "xn--5su34j936bgsg", "xn--5tzm5g", "xn--6frz82g", "xn--6qq986b3xl",
        "xn--80adxhks", "xn--80ao21a", "xn--80aqecdr1a", "xn--80asehdb",
        "xn--80aswg", "xn--8y0a063a", "xn--90a3ac", "xn--90ae", "xn--90ais",
        "xn--9dbq2a", "xn--9et52u", "xn--9krt00a", "xn--b4w605ferd",
        "xn--bck1b9a5dre4c", "xn--c1avg", "xn--c2br7g", "xn--cck2b3b",
        "xn--cckwcxetd", "xn--cg4bki", "xn--clchc0ea0b2g2a9gcd", "xn--czr694b",
        "xn--czrs0t", "xn--czru2d", "xn--d1acj3b", "xn--d1alf", "xn--e1a4c",
        "xn--eckvdtc9d", "xn--efvy88h", "xn--fct429k", "xn--fhbei",
        "xn--fiq228c5hs", "xn--fiq64b", "xn--fiqs8s", "xn--fiqz9s",
        "xn--fjq720a", "xn--flw351e", "xn--fpcrj9c3d", "xn--fzc2c9e2c",
        "xn--fzys8d69uvgm", "xn--g2xx48c", "xn--gckr3f0f", "xn--gecrj9c",
        "xn--gk3at1e", "xn--h2breg3eve", "xn--h2brj9c", "xn--h2brj9c8c",
        "xn--hxt814e", "xn--i1b6b1a6a2e", "xn--imr513n", "xn--io0a7i",
        "xn--j1aef", "xn--j1amh", "xn--j6w193g", "xn--jlq480n2rg",
        "xn--jvr189m", "xn--kcrx77d1x4a", "xn--kprw13d", "xn--kpry57d",
        "xn--kput3i", "xn--l1acc", "xn--lgbbat1ad8j", "xn--mgb9awbf",
        "xn--mgba3a3ejt", "xn--mgba3a4f16a", "xn--mgba7c0bbn0a",
        "xn--mgbaakc7dvf", "xn--mgbaam7a8h", "xn--mgbab2bd",
        "xn--mgbah1a3hjkrd", "xn--mgbai9azgqp6j", "xn--mgbayh7gpa",
        "xn--mgbbh1a", "xn--mgbbh1a71e", "xn--mgbc0a9azcg", "xn--mgbca7dzdo",
        "xn--mgbcpq6gpa1a", "xn--mgberp4a5d4ar", "xn--mgbgu82a",
        "xn--mgbi4ecexp", "xn--mgbpl2fh", "xn--mgbt3dhd", "xn--mgbtx2b",
        "xn--mgbx4cd0ab", "xn--mix891f", "xn--mk1bu44c", "xn--mxtq1m",
        "xn--ngbc5azd", "xn--ngbe9e0a", "xn--ngbrx", "xn--node", "xn--nqv7f",
        "xn--nqv7fs00ema", "xn--nyqy26a", "xn--o3cw4h", "xn--ogbpf8fl",
        "xn--otu796d", "xn--p1acf", "xn--p1ai", "xn--pgbs0dh", "xn--pssy2u",
        "xn--q7ce6a", "xn--q9jyb4c", "xn--qcka1pmc", "xn--qxa6a", "xn--qxam",
        "xn--rhqv96g", "xn--rovu88b", "xn--rvc1e0am3e", "xn--s9brj9c",
        "xn--ses554g", "xn--t60b56a", "xn--tckwe", "xn--tiq49xqyj",
        "xn--unup4y", "xn--vermgensberater-ctb", "xn--vermgensberatung-pwb",
        "xn--vhquv", "xn--vuq861b", "xn--w4r85el8fhu5dnra", "xn--w4rs40l",
        "xn--wgbh1c", "xn--wgbl6a", "xn--xhq521b", "xn--xkc2al3hye2a",
        "xn--xkc2dl3a5ee0h", "xn--y9a3aq", "xn--yfro4i67o", "xn--ygbi2ammx",
        "xn--zfr164b", "xxx", "xyz", "yachts", "yahoo", "yamaxun", "yandex",
        "ye", "yodobashi", "yoga", "yokohama", "you", "youtube", "yt", "yun",
        "za", "zappos", "zara", "zero", "zip", "zm", "zone", "zuerich", "zw"
    };

    if (tlds.count(tld) > 0) {
        return true;
    } else {
        return false;
    }
}

bool is_icann_name(std::string plaintext_name) {
    // At most, a single domain seperator (.)
    // Each label must be in LDH form, and between 1-63 octets, inclusive
    // This gives a maximum length of 127 octets (including the seperator)
    // The final label must be an ICANN TLD.
    if (plaintext_name.size() < 1 || plaintext_name.size() > 127)
        return false;
    std::vector<std::string> labels;
    boost::split(labels, plaintext_name, boost::is_any_of("."), boost::token_compress_off);
    if (labels.size() < 1 || labels.size() > 2)
        return false;
    std::regex ldhRegex("^[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9]$");
    for (const auto& label : labels) {
        if (label.size() < 1 || label.size() > 63)
            return false;
        if (!std::regex_match(label, ldhRegex))
            return false;
    }
    if (!is_icann_tld(labels.back()))
        return false;
    return true;
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
    bool fBitNameRegisterIcann =
        tx.nVersion == TRANSACTION_BITNAME_REGISTER_ICANN_VERSION;

    // Create BitName transactions must have at least 1 output
    if (fCreateBitName && tx.vout.size() < 1)
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-create-bitname-vout-size");
    // ICANN BitName registrations must have a valid signature over the name hash and first output
    if (fBitNameRegistration && tx.fIcann) {
        // FIXME: this looks sus
        uint256 registration_output_hash = SerializeHash(tx.vout[0]);
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

    // Register Icann transactions must have at least as many outputs
    // as registrations.
    // Each registration must be for a unique icann-formatted name.
    if (fBitNameRegisterIcann) {
        if (tx.vout.size() < tx.icann_registrations.size())
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-register-icann-bitname-vout-size");
        for (const auto& registration : tx.icann_registrations) {
            if(!is_icann_name(registration))
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-register-icann-invalid-name");
        }
        // check that registrations are unique by name hash
        std::vector<uint256> name_hashes =
            std::vector<uint256>(tx.icann_registrations.size(), uint256());
        std::transform(
            tx.icann_registrations.begin(),
            tx.icann_registrations.end(),
            name_hashes.begin(),
            [](const std::string& registration){
                uint256 hash_result;
                CHash256().Write(
                    (unsigned char*) registration.data(),
                    registration.size()
                )
                .Finalize((unsigned char*) &hash_result);
                return hash_result;
            }
        );
        std::sort(name_hashes.begin(), name_hashes.end());
        bool isUnique = std::adjacent_find(name_hashes.begin(), name_hashes.end()) == name_hashes.end();
        if (!isUnique)
            return state.DoS(10, false, REJECT_INVALID, "bad-txns-register-icann-bad-registrations");
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
    } else if (fBitNameRegisterIcann) {
        // The first `tx.icann_registrations.size()` outputs must have a value of 1
        for (; it < tx.vout.begin() + tx.icann_registrations.size(); it++) {
            if (it->nValue > 1)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
            else if (it->nValue < 0)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        }
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
    bool fBitNameRegisterIcann =
        tx.nVersion == TRANSACTION_BITNAME_REGISTER_ICANN_VERSION;

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

    // in a register icann bitname tx, there MAY exist bitname registration
    // inputs at the start of the inputs.
    // each of these bitname inputs must correspond to a registration, in order.
    // There must exist a valid authenticated signature over the following data:
    // * nVersion
    // * hash of outpoints for Bitname tx inputs
    // * hash of registration outputs
    // * nLockTime
    // * hash of icann registrations
    if (fBitNameRegisterIcann) {
        // check that each bitname input corresponds to a registration,
        // up until the first bitcoin input.
        // After the first bitcoin input, no bitname inputs are permitted.
        bool bitcoin_input_found = false;
        std::vector<COutPoint> bitname_outpoints = {};
        for (unsigned int vin_idx = 0; vin_idx < tx.vin.size(); ++vin_idx) {
            const COutPoint &prevout = tx.vin[vin_idx].prevout;
            const Coin& inputCoin = inputs.AccessCoin(prevout);
            if (inputCoin.fBitName) {
                // check that no bitcoin input has been found so far
                if (bitcoin_input_found)
                    return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-unexpected-bitname");
                // check that registration exists and has the correct name
                if (vin_idx >= tx.icann_registrations.size())
                    return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-missing-registration");
                std::string plaintext_name = tx.icann_registrations[vin_idx];
                uint256 hash_result;
                CHash256().Write(
                    (unsigned char*) plaintext_name.data(),
                    plaintext_name.size()
                )
                .Finalize((unsigned char*) &hash_result);
                if (hash_result != inputCoin.nAssetID)
                    return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-wrong-registration");
                bitname_outpoints.push_back(prevout);
            } else {
                bitcoin_input_found = true;
            }
        }
        // construct the hash to sign and check the auth signature
        uint256 auth_hash = uint256();
        CSHA256 hasher = CSHA256();
        std::vector<uint8_t> nVersion(sizeof(int32_t));
        std::memcpy(nVersion.data(), &tx.nVersion, sizeof(int32_t));
        hasher.Write(nVersion.data(), nVersion.size());
        uint256 bitname_outpoints_hash = SerializeHash(bitname_outpoints);
        hasher.Write(
            bitname_outpoints_hash.begin(),
            bitname_outpoints_hash.size()
        );
        std::vector<CTxOut> registration_outputs(
            tx.vout.begin(),
            tx.vout.begin() + tx.icann_registrations.size()
        );
        uint256 registration_outputs_hash = SerializeHash(registration_outputs);
        hasher.Write(
            registration_outputs_hash.begin(),
            registration_outputs_hash.size()
        );
        std::vector<uint8_t> nLockTime(sizeof(uint32_t));
        std::memcpy(nLockTime.data(), &tx.nLockTime, sizeof(uint32_t));
        hasher.Write(nLockTime.data(), nLockTime.size());
        uint256 registrations_hash = SerializeHash(tx.icann_registrations);
        hasher.Write(
            registrations_hash.begin(),
            registrations_hash.size()
        );
        hasher.Finalize((unsigned char*) &auth_hash);
        // see if we can recover a pubkey from the signature
        CPubKey pubkey = CPubKey();
        std::vector<uint8_t> vchSig =
            std::vector<uint8_t>(tx.icann_sig.begin(), tx.icann_sig.end());
        if (!pubkey.RecoverCompact(auth_hash, vchSig)) {
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
        if (!pubkey.Verify(auth_hash, der_sig)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-icann-sig");
        }
        // FIXME: require that the recovered pubkey matches the authenticated PKH

    }

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
        if (coin.nAssetID == uint256())
            nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }

        if (fCreateBitName || fUpdateBitName) {
            // may only be the last input in a create or update bitname tx
            if (coin.nAssetID != uint256() && (i != tx.vin.size() - 1)) {
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-inputs-unexpected-assets");
            }
        }
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
