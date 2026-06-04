# Third-party dependencies

Chronos supports pinning test dependencies in-tree for reproducible and offline builds.

## Catch2

Preferred location: `third_party/Catch2`.

Typical setup (submodule):

```bash
git submodule add https://github.com/catchorg/Catch2.git third_party/Catch2
git -C third_party/Catch2 checkout v3.5.2
```

If `third_party/Catch2` exists, CMake will use it before trying `find_package` or network fetch.
