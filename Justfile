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
    rm -rf build compile_commands.json

run: build
    build/chronos.exe

arch-lint:
    python3 scripts/check_layer_deps.py


arch-lint-strict:
    python3 scripts/check_layer_deps.py --strict

arch-lint-baseline:
    python3 scripts/check_layer_deps.py --fail-on-stale-baseline
