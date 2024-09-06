# avbroot-helper
#### This tool make rom patching with custom keys easier
#### Helpful for using root on locked bootloader
# Compiling guide (windows)
#### 1) Download compiler - https://winlibs.com (need with posix threads)
#### 2) Add it to path
#### 3) Compile
`gcc main.cpp -o main.exe`
#### Soon will add guide to compile with cmake
# Compiling guide (linux)
#### 1) Install gcc, make, cmake
#### 2) Compile
`cmake . && make -j$(nproc)`
