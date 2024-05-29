#pragma once

#include <TrustWalletCore/TWAnySigner.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWCoinTypeConfiguration.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWString.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <atomic>
#include <functional>
#include <iomanip>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using umap_s_vs = std::unordered_map<std::string, std::vector<std::string>>;
using umap_s_TWCT = std::unordered_map<std::string, TWCoinType>;

enum EngineCMD { GET_TOTAL_SCANNED, SHOW_GENERATED_WALLET, STOP };

class ScanEngine {
  private:
    std::atomic<size_t> _counter;
    size_t _nThreads;
    std::shared_mutex _mutx;

    std::vector<std::thread> _vecThreads;
    umap_s_vs _mapWallets;
    umap_s_TWCT _mTWCT;

    bool _isRunning;
    bool _isShowWallet;

    void _generateWalets();
    void _generateAWalet(TWHDWallet* walletNew, const std::string& typeCoin,
                         const std::vector<boost::iostreams::mapped_file>& listMappedFiles);

    bool _bFindWallet(const boost::iostreams::mapped_file& mmap, const std::string& wallet);
    void _writeFoundFile(const char* mnemonic);

  public:
    ScanEngine() = delete;
    ScanEngine(const boost::property_tree::ptree& pt, const umap_s_vs& mapWallet);
    ~ScanEngine();

    void run();
    void listen();
    void stop();
    umap_s_vs getMWallet();

    template <typename T>
    T command(EngineCMD cmd);
};

template <typename T>
T ScanEngine::command(EngineCMD cmd) {
    switch (cmd) {
    case GET_TOTAL_SCANNED:
        return _counter;
        break;

    case SHOW_GENERATED_WALLET:
        _isShowWallet = true;
        break;

    case STOP: {
        unique_lock<shared_mutex> mtx(_mutx);
        _isRunning = false;
    }

    break;

    default:
        break;
    }

    return 0;
}
