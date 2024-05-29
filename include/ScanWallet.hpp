#pragma once

#include "ScanEngine.hpp"

#include <boost/algorithm/string.hpp>

class ScanWallet {
  private:
    ScanEngine _engine;
    bool _isLifetime;
    std::string _expirationDate;
	static std::unordered_map<std::string, std::pair<size_t, size_t>> _mapWallet;

    const boost::property_tree::ptree& _pt;

    static bool _isRunning;
    static umap_s_vs _mWalletHandleFirst(const boost::property_tree::ptree& pt);

    void _printTitle();

    bool _check_hwid(size_t& phwid);
    bool _parseKey(std::vector<std::string>& keyParsed);
    bool _check_license(const size_t& device_id, const std::vector<std::string>& keyParsed);

    void _animated();
    void _checkNumberWallet();
	void _printWalletSize();

  public:
    ScanWallet() = delete;
    ScanWallet(const boost::property_tree::ptree& pt);
    ~ScanWallet();

    void run();
    void listen();
};
