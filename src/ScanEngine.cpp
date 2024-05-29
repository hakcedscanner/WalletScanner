#include "ScanEngine.hpp"

using namespace std;

ScanEngine::ScanEngine(const boost::property_tree::ptree& pt,
                       const unordered_map<string, vector<string>>& mapWallet)
    : _counter(0)
    , _nThreads{pt.get_optional<size_t>("thread_number").get_value_or(1)}
    , _isRunning{true}
    , _mapWallets{mapWallet}
    , _mTWCT{{"eth_wallets", TWCoinType::TWCoinTypeEthereum},
             {"btc_P2SH_wallets", TWCoinType::TWCoinTypeBitcoin},
             {"ltc_wallets", TWCoinType::TWCoinTypeLitecoin},
             {"doge_wallets", TWCoinType::TWCoinTypeDogecoin},}
    , _isShowWallet{false} {}

ScanEngine::~ScanEngine() {
    for (auto& thread : _vecThreads) {
        thread.join();
    }
}

void ScanEngine::run() {
    _vecThreads.reserve(_nThreads);
    for (size_t i = 0; i < _nThreads; ++i) {
        _vecThreads.emplace_back(thread(&ScanEngine::_generateWalets, this));
    }
}
void ScanEngine::listen() {}
umap_s_vs ScanEngine::getMWallet() {
    return _mapWallets;
}

void ScanEngine::_writeFoundFile(const char* mnemonic) {
    ofstream file("found.txt", ios::app);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file.\n";
        return;
    }
    file << mnemonic << endl;
    file.close();
}

void ScanEngine::_generateAWalet(TWHDWallet* walletNew, const string& typeCoin,
                                 const vector<boost::iostreams::mapped_file>& listMappedFiles) {

    for (const boost::iostreams::mapped_file& wallet : listMappedFiles) {

        string walletAddress =
            TWStringUTF8Bytes(TWHDWalletGetAddressForCoin(walletNew, _mTWCT[typeCoin]));

        bool check = _bFindWallet(wallet, walletAddress);
        // bool check = false;
        if (check) {
            _writeFoundFile(TWStringUTF8Bytes(TWHDWalletMnemonic(walletNew)));
        }
        _counter.fetch_add(1);

        if (_isShowWallet) {
            unique_lock<shared_mutex> mtx(_mutx);
            cout << setw(12) << _counter << ":" << setw(9) << (check ? "Found" : "Not found") << ":"
                 << setw(20) << typeCoin << ":" << setw(45) << walletAddress << ":  "
                 << TWStringUTF8Bytes(TWHDWalletMnemonic(walletNew)) << endl;
        }
    }
}

void ScanEngine::_generateWalets() {

    vector<pair<string, vector<boost::iostreams::mapped_file>>> pStringlistMappedFiles;

    for (const auto& [key, value] : _mapWallets) {
        if (!value.empty()) {
            vector<boost::iostreams::mapped_file> vecMappedFile;
            for (const string& wallet : value) {
                vecMappedFile.push_back(
                    boost::iostreams::mapped_file(wallet, boost::iostreams::mapped_file::readonly));
            }
            pStringlistMappedFiles.push_back(make_pair(key, vecMappedFile));
        }
    }

    while ([&]() {
        shared_lock<shared_mutex> mtx(_mutx);
        return _isRunning;
    }()) {
        TWHDWallet* walletNew = TWHDWalletCreate(128, TWStringCreateWithUTF8Bytes(""));
        for (const auto& [key, value] : pStringlistMappedFiles) {
            ScanEngine::_generateAWalet(walletNew, key, value);
        }
        delete walletNew;
    }
}

bool ScanEngine::_bFindWallet(const boost::iostreams::mapped_file& mmap, const string& wallet) {
    auto left = mmap.const_data();
    size_t len_wallet = wallet.size() + 2;
    size_t c_lenwallet = len_wallet - 2;

    auto right = left + mmap.size() - len_wallet;
    size_t number_of_wallets = (right - left) / len_wallet;
    const char* c_wallet = wallet.c_str();
    while (left <= right) {
        auto middle = left + (((right - left) / len_wallet) / 2) * len_wallet;

        if (strncmp(middle, c_wallet, c_lenwallet) == 0) {
            return true;
        }
        if (strncmp(middle, c_wallet, c_lenwallet) < 0) {
            left = middle + len_wallet;
        }

        else {
            right = middle - len_wallet;
        }
    }
    return false;
}
