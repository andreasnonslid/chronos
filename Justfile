cmake := env("CMAKE", "cmake")
cxx   := env("CXX", "clang++")

build:
    {{cmake}} --preset release -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build

test:
    {{cmake}} --preset test -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build
    cd build && ctest --output-on-failure

debug:
    {{cmake}} --preset debug -DCMAKE_CXX_COMPILER={{cxx}}
    {{cmake}} --build build

clean:
    rm -rf build

run: build
    build/chronos.exe
