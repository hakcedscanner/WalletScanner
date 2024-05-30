# About
This is a tool to scan seed phrases for crypto wallets, supporting a license system.

# Build
To build the program, first need to build the source code of the wallet-core:<br />
https://github.com/trustwallet/wallet-core.git

For windows, refer to build wallet-core here:<br />
https://github.com/kaetemi/wallet-core-windows

```cmake
mkdir build
cd build
cmake "-DCMAKE_BUILD_TYPE=Release" "-DWALLET_CORE=$<path to root built wallet-core>" ..
cmake --build . --config Release
```

or

If you use the windows version of wallet-core, you can put the project in its `sample/cpp` path and run `windows-samples.ps1` from the root path of wallet-core

```powersell
PS C:\...\wallet-core-windows > pwd
Path
----
C:\...\wallet-core-windows

PS C:\...\wallet-core-windows > .\tools\windows-samples.ps1
```
