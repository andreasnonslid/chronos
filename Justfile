cmake := "C:/msys64/mingw64/bin/cmake.exe"
cxx   := "C:/msys64/mingw64/bin/clang++.exe"

build:
    {{cmake}} -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build

run: build
    build/chronos.exe
