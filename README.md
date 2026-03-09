# MyLogReader (Qt + CMake + MinGW)

TODO write description

## Prerequisites
- Qt 6 (MinGW 64-bit) + CMake 3.21+
- Environment variable `QT_ROOT` pointing to your Qt prefix, e.g.
  `QT_ROOT=C:\Qt\6.10.2\mingw_64`

> Qt 6 requires a C++17-capable compiler; CMake config enforces that.  
> (This repo uses Qt's recommended CMake commands for Qt 6.)  

## Build, run, install

```powershell
# Configure + build (Debug or Release)
cmake --preset mingw-debug
cmake --build --preset debug -j

# or
cmake --preset mingw-release
cmake --build --preset release -j

# Install to a self-contained folder (copies Qt DLLs via windeployqt)
cmake --install build/mingw-release
# Result: install/mingw-release/bin/hello_qt.exe   (ready to run/zip)
