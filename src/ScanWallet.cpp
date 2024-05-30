#include "hwid.hpp"

#include "ScanWallet.hpp"
#include "Utils.hpp"
#include "encrypt.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif // _WIN32

using namespace std;
using namespace chrono;
bool ScanWallet::_isRunning{false};
unordered_map<string, pair<size_t, size_t>> ScanWallet::_mapWallet{{"eth_wallets", {44, 0}},
                                                                   {"btc_P2WPKH_wallets", {36, 0}},
                                                                   {"ltc_wallets", {45, 0}},
                                                                   {"doge_wallets", {36, 0}}};

umap_s_vs ScanWallet::_mWalletHandleFirst(const boost::property_tree::ptree& pt) {

    unordered_map<string, vector<string>> returnWallet;

    for (auto& [_mapKey, _mapValue] : _mapWallet) {
        vector<string> vWallet(to_array<string>(pt.get_optional<string>(_mapKey).get_value_or({})));
        for (auto& wallet : vWallet)
            if (!filesystem::exists(wallet)) {
                cout << "Unable to open file: " << wallet << endl;
                return {};
            } else {
                _mapValue.second += count_line(
                    boost::iostreams::mapped_file(wallet, boost::iostreams::mapped_file::readonly),
                    _mapValue.first);
            }

        if (!vWallet.empty()) {
            returnWallet.insert({_mapKey, vWallet});
        }
    }

    return returnWallet;
}

ScanWallet::ScanWallet(const boost::property_tree::ptree& pt)
    : _pt(pt), _engine(pt, ScanWallet::_mWalletHandleFirst(pt)), _isLifetime(false) {

    _printTitle();

    size_t hwid;
    if (!_check_hwid(hwid))
        return;

    vector<string> keyParsed;
    if (!_parseKey(keyParsed)) {
        cout << "Invalid key !" << endl;
        return;
    }

    if (!_check_license(hwid, keyParsed))
        return;

    if (_engine.getMWallet().empty()) {
        cout << "No wallet listings have been set!" << endl;
        return;
    }

    _printWalletSize();
    _isRunning = true;
    _engine.run();
}

ScanWallet::~ScanWallet() {
    // _engine.stop();
}

void ScanWallet::run() {
    if (!_isRunning) {
        cin.get();
        return;
    }

    // _engine.command<int>(SHOW_GENERATED_WALLET);

    if (_pt.get<string>("print_all_wallet") == "on") {
        _engine.command<int>(SHOW_GENERATED_WALLET);
        this_thread::sleep_for(chrono::seconds(_pt.get<int>("second_print_all_wallet")));
        _engine.command<int>(STOP);
        this_thread::sleep_for(chrono::milliseconds(1000));
        cout << "Done !" << endl;
        std::cin.get();
        return;
    }

    size_t milisecDelay = 500;
    size_t temp = 0;

    while (true) {
        auto start = high_resolution_clock::now();
        if (!_isLifetime) {
            if (!check_date_licence(_expirationDate)) {
                return;
            }
        }
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        size_t calc;
        if (chrono::milliseconds(milisecDelay) > duration) {
            auto delay = chrono::milliseconds(milisecDelay) - duration;
            this_thread::sleep_for(delay);
            calc = milisecDelay;
        } else {
            calc = duration.count();
        }

        _animated();
        size_t total = _engine.command<size_t>(GET_TOTAL_SCANNED);
        size_t speed = (total - temp) * (double)(1000 / calc);
        cout << "Total: " << setw(15) << total << " | Speed: " << speed << " address/sec" << flush;
        temp = total;
    }
}

void ScanWallet::listen() {}

void ScanWallet::_printTitle() {
    cout << R"(  _    _       _                _    _____                                 
 | |  | |     | |              | |  / ____|                                
 | |__| | __ _| | _____ ___  __| | | (___   ___ __ _ _ __  _ __   ___ _ __ 
 |  __  |/ _` | |/ / __/ _ \/ _` |  \___ \ / __/ _` | '_ \| '_ \ / _ \ '__|
 | |  | | (_| |   < (_|  __/ (_| |  ____) | (_| (_| | | | | | | |  __/ |   
 |_|  |_|\__,_|_|\_\___\___|\__,_| |_____/ \___\__,_|_| |_|_| |_|\___|_|                                                                            
)" << flush;
    cout << "Version: 3.0.2" << endl;
}

bool ScanWallet::_parseKey(vector<string>& keyParsed) {
    try {
        string ss(decrypt(_pt.get<string>("license_key"), "This key has been changed :D"));
        vector<string> strs;
        boost::split(strs, ss, boost::is_any_of("|"));
        keyParsed = strs;
        return strs.size() >= 2;
    } catch (...) {
        return false;
    }
}

bool ScanWallet::_check_hwid(size_t& phwid) {
    try {
#ifdef _WIN32
        HardwareId hwid;
        hash<wstring> hashWstring;
        size_t haser = hashWstring(
            hwid.System.Name + hwid.System.OSArchitecture + hwid.System.OSSerialNumber +
            hwid.System.OSVersion + hwid.SMBIOS.Manufacturer + hwid.SMBIOS.Product +
            hwid.SMBIOS.SerialNumber + hwid.SMBIOS.Version + hwid.Registry.ComputerHardwareId);
        SetConsoleTextAttribute(hConsole, 14);

        cout << "Device id: " << flush;

#ifdef _WIN32
        SetConsoleTextAttribute(hConsole, 15);
#endif // _WIN32

#elif defined(__linux__)

        hash<string> hashString;
        array<char, 128> buffer;
        string result;
        unique_ptr<FILE, decltype(&pclose)> pipe(
            popen("sudo lshw -c processor -c memory -json", "r"), pclose);
        if (!pipe) {
            cout << "Cannot read hardware info !" << endl;
            return 0;
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        if (result.empty()) {
            cout << "Cannot read hardware info !" << result << endl;
            return 0;
        }
        size_t haser = hashString(result);
#endif // __linux__
        phwid = haser;
        cout << haser << endl;
        return true;
    } catch (...) {
        cout << "Error during hwid check" << endl;
        return false;
    }
}

bool ScanWallet::_check_license(const size_t& device_id, const vector<string>& keyParsed) {
    // Check hwid

    bool isExpired = true;
    stringstream ss(keyParsed[0]);
    size_t sst;
    ss >> sst;

    if (device_id != sst) {
        cout << "Your device has changed !" << endl;
        cin.get();
        return false;
    }

    // Check licence
    if (keyParsed[1] != "lifetime") {
        if (check_date_licence(keyParsed[1])) {
            isExpired = false;
        }
    } else {
        _isLifetime = true;
        isExpired = false;
    }

#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, 14);
#endif // _WIN32

    std::cout << "Expire time: ";

#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, 15);
#endif // _WIN32
    std::cout << keyParsed[1] << std::endl;
    _expirationDate = keyParsed[1];

    if (isExpired) {
        std::cout << "Your key is expired !" << std::endl;
        return false;
    }

    return true;
}

void ScanWallet::_animated() {
    std::vector<char*> animate = {
        {"[      ]"}, {"[=     ]"}, {"[==    ]"}, {"[===   ]"}, {"[====  ]"}, {"[===== ]"},
        {"[======]"}, {"[ =====]"}, {"[  ====]"}, {"[   ===]"}, {"[    ==]"}, {"[     =]"},
    };

    static size_t animated_count = 0;
    if (animated_count == animate.size())
        animated_count = 0;

    std::cout << '\r';
    std::cout << (animate[animated_count++]) << " ";
}

void ScanWallet::_printWalletSize() {
    std::cout << "------------------------------" << std::endl;
    for (const auto& [_key, _value] : _mapWallet) {
        std::cout << _key << ": " << _value.second << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}