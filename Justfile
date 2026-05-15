cmake := env("CMAKE", "cmake")
cxx   := env("CXX", "clang++")

build:
    {{cmake}} --preset release -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build

run: build
    build/chronos.exe
