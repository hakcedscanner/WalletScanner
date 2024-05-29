#pragma once


#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <ntddscsi.h>

#pragma comment(lib, "wbemuuid.lib")

class HardwareId {

  public:
    struct VolumeObject {

        unsigned long SerialNumber{};
        std::wstring DriveLetter{};
        std::wstring Name{};
        long long Size{};
        long long FreeSpace{};
    };

    struct DiskObject {

        std::wstring SerialNumber{};
        std::wstring Name;
        std::wstring Model{};
        std::wstring Interface{};
        std::wstring DriveLetter{};
        long long Size{};
        long long FreeSpace{};
        unsigned int MediaType{};
        unsigned int BusType{};
        bool IsBootDrive{};
        std::vector<VolumeObject> Volumes{};
    };
    std::vector<DiskObject> Disk{};

    struct {

        std::wstring Manufacturer{};
        std::wstring Product{};
        std::wstring Version{};
        std::wstring SerialNumber{};

    } SMBIOS;

    struct {

        std::wstring ProcessorId{};
        std::wstring Manufacturer{};
        std::wstring Name{};
        int Cores{};
        int Threads{};

    } CPU;

    struct GPUObject {

        std::wstring Name{};
        std::wstring DriverVersion{};
        unsigned long long Memory{};
        int XResolution{};
        int YResolution{};
        int RefreshRate{};
    };
    std::vector<GPUObject> GPU{};

    struct NetworkAdapterObject {

        std::wstring Name{};
        std::wstring MAC{};
    };
    std::vector<NetworkAdapterObject> NetworkAdapter{};

    struct {

        std::wstring Name{};
        bool IsHypervisorPresent{};
        std::wstring OSVersion{};
        std::wstring OSName{};
        std::wstring OSArchitecture{};
        std::wstring OSSerialNumber{};

    } System;

    struct {

        std::wstring PartNumber{};

    } PhysicalMemory;

    struct {

        std::wstring ComputerHardwareId{};

    } Registry;

    std::unique_ptr<HardwareId> Pointer() { return std::make_unique<HardwareId>(*this); }

    HardwareId() { GetHardwareId(); }

    static std::wstring SafeString(const wchar_t* pString) {
        return std::wstring((pString == nullptr ? L"(null)" : pString));
    }

    static void RemoveWhitespaces(std::wstring& String) {
        String.erase(std::remove(String.begin(), String.end(), L' '), String.end());
    }

  private:
    std::wstring GetHKLM(std::wstring SubKey, std::wstring Value) {
        DWORD Size{};
        std::wstring Ret{};

        RegGetValueW(HKEY_LOCAL_MACHINE, SubKey.c_str(), Value.c_str(), RRF_RT_REG_SZ, nullptr,
                     nullptr, &Size);

        Ret.resize(Size);

        RegGetValueW(HKEY_LOCAL_MACHINE, SubKey.c_str(), Value.c_str(), RRF_RT_REG_SZ, nullptr,
                     &Ret[0], &Size);

        return Ret.c_str();
    }

    template <typename T = const wchar_t*>
    void QueryWMI(std::wstring WMIClass, std::wstring Field, std::vector<T>& Value,
                  const wchar_t* ServerName = L"ROOT\\CIMV2") {
        std::wstring Query(L"SELECT ");
        Query.append(Field.c_str()).append(L" FROM ").append(WMIClass.c_str());

        IWbemLocator* Locator{};
        IWbemServices* Services{};
        IEnumWbemClassObject* Enumerator{};
        IWbemClassObject* ClassObject{};
        VARIANT Variant{};
        DWORD Returned{};

        HRESULT hResult{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};

        if (FAILED(hResult)) {
            return;
        }

        hResult = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
                                       RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

        if (FAILED(hResult)) {
            CoUninitialize();
            return;
        }

        hResult = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                   reinterpret_cast<PVOID*>(&Locator));

        if (FAILED(hResult)) {
            CoUninitialize();
            return;
        }

        hResult = Locator->ConnectServer(_bstr_t(ServerName), nullptr, nullptr, nullptr, NULL,
                                         nullptr, nullptr, &Services);

        if (FAILED(hResult)) {
            Locator->Release();
            CoUninitialize();
            return;
        }

        hResult = CoSetProxyBlanket(Services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                                    RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
                                    EOAC_NONE);

        if (FAILED(hResult)) {
            Services->Release();
            Locator->Release();
            CoUninitialize();
            return;
        }

        hResult = Services->ExecQuery(bstr_t(L"WQL"), bstr_t(Query.c_str()),
                                      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                      nullptr, &Enumerator);

        if (FAILED(hResult)) {
            Services->Release();
            Locator->Release();
            CoUninitialize();
            return;
        }

        while (Enumerator) {

            HRESULT Res = Enumerator->Next(WBEM_INFINITE, 1, &ClassObject, &Returned);

            if (!Returned) {
                break;
            }

            Res = ClassObject->Get(Field.c_str(), 0, &Variant, nullptr, nullptr);

            if (typeid(T) == typeid(long) || typeid(T) == typeid(int)) {
                Value.push_back((T)Variant.intVal);
            } else if (typeid(T) == typeid(bool)) {
                Value.push_back((T)Variant.boolVal);
            } else if (typeid(T) == typeid(unsigned int)) {
                Value.push_back((T)Variant.uintVal);
            } else if (typeid(T) == typeid(unsigned short)) {
                Value.push_back((T)Variant.uiVal);
            } else if (typeid(T) == typeid(long long)) {
                Value.push_back((T)Variant.llVal);
            } else if (typeid(T) == typeid(unsigned long long)) {
                Value.push_back((T)Variant.ullVal);
            } else {
                Value.push_back((T)((bstr_t)Variant.bstrVal).copy());
            }

            VariantClear(&Variant);
            ClassObject->Release();
        }

        if (!Value.size()) {
            Value.resize(1);
        }

        Services->Release();
        Locator->Release();
        Enumerator->Release();
        CoUninitialize();
    }


    void QuerySMBIOS() {

        std::vector<const wchar_t*> Manufacturer{};
        std::vector<const wchar_t*> Product{};
        std::vector<const wchar_t*> Version{};
        std::vector<const wchar_t*> SerialNumber{};

        QueryWMI(L"Win32_BaseBoard", L"Manufacturer", Manufacturer);
        QueryWMI(L"Win32_BaseBoard", L"Product", Product);
        QueryWMI(L"Win32_BaseBoard", L"Version", Version);
        QueryWMI(L"Win32_BaseBoard", L"SerialNumber", SerialNumber);

        this->SMBIOS.Manufacturer = SafeString(Manufacturer.at(0));
        this->SMBIOS.Product = SafeString(Product.at(0));
        this->SMBIOS.Version = SafeString(Version.at(0));
        this->SMBIOS.SerialNumber = SafeString(SerialNumber.at(0));
    }

    void QuerySystem() {

        std::vector<const wchar_t*> SystemName{};
        std::vector<const wchar_t*> OSVersion{};
        std::vector<const wchar_t*> OSName{};
        std::vector<const wchar_t*> OSArchitecture{};
        std::vector<const wchar_t*> OSSerialNumber{};
        std::vector<bool> IsHypervisorPresent{};

        QueryWMI(L"Win32_ComputerSystem", L"Name", SystemName);
        QueryWMI(L"Win32_ComputerSystem", L"HypervisorPresent", IsHypervisorPresent);
        QueryWMI(L"Win32_OperatingSystem", L"Version", OSVersion);
        QueryWMI(L"Win32_OperatingSystem", L"Name", OSName);
        QueryWMI(L"Win32_OperatingSystem", L"OSArchitecture", OSArchitecture);
        QueryWMI(L"Win32_OperatingSystem", L"SerialNumber", OSSerialNumber);

        std::wstring wOSName{SafeString(OSName.at(0))};

        if (wOSName.find('|') != std::wstring::npos) {
            wOSName.resize(wOSName.find('|'));
        }

        this->System.Name = SafeString(SystemName.at(0));
        this->System.IsHypervisorPresent = IsHypervisorPresent.at(0);
        this->System.OSName = wOSName;
        this->System.OSVersion = SafeString(OSVersion.at(0));
        this->System.OSSerialNumber = SafeString(OSSerialNumber.at(0));
        this->System.OSArchitecture = SafeString(OSArchitecture.at(0));
    }




    void QueryRegistry() {
        this->Registry.ComputerHardwareId = SafeString(
            GetHKLM(L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId")
                .c_str());
    }

    void GetHardwareId() {
        // QueryDisk();
        QuerySMBIOS();
        // QueryProcessor();
        // QueryGPU();
        QuerySystem();
        // QueryNetwork();
        // QueryPhysicalMemory();
        QueryRegistry();
    }
};
