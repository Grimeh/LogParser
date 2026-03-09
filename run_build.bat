set "PATH=C:\Qt\Tools\mingw1310_64\bin;%PATH%"
set "QT_ROOT=C:\Qt\6.10.2\mingw_64"

cmake --preset mingw-release
cmake --build --preset release -j
cmake --install build/mingw-release