cmake := "C:/msys64/mingw64/bin/cmake.exe"
cxx   := "C:/msys64/mingw64/bin/clang++.exe"

build:
    {{cmake}} --preset release -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build

run: build
    build/chronos.exe
