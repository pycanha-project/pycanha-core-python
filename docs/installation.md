# Installation

## From PyPI

```bash
pip install pycanha-core
```

Wheels are published for Python 3.13 and later on Windows, Linux and macOS.

## From source

Check out `pycanha-core` and `pycanha-core-python` side by side:

```text
pycanha-project/
├── pycanha-core/
└── pycanha-core-python/
```

The build needs Conan, CMake and a compiler with C++23 support. The bundled
Conan profile selects GCC 15 on Linux, MSVC 19.5x on Windows and Apple Clang 21
on macOS, matching the compilers used by the CI.

```bash
cd pycanha-project
conan export ./pycanha-core
conan install ./pycanha-core-python/src/pycanha_core_bindings \
  -pr:h=./pycanha-core-python/src/pycanha_core_bindings/pycanha-core-conan-profile \
  -pr:b=./pycanha-core-python/src/pycanha_core_bindings/pycanha-core-conan-profile \
  --build=pycanha-core/* \
  --build=missing \
  -of ./pycanha-core-python/build/conan-deps \
  -o pycanha-core*:PYCANHA_OPTION_USE_MKL=True

cd pycanha-core-python
CC=gcc-15 CXX=g++-15 pip install . \
  --config-settings=cmake.define.CMAKE_TOOLCHAIN_FILE="$PWD/build/conan-deps/conan_toolchain.cmake"
```

`CC` and `CXX` select the same compiler as the Conan profile. On Windows, run
the `pip install` from a shell with the MSVC environment loaded instead. On
macOS, use `CC=clang CXX=clang++`.

`conan export` makes the local `pycanha-core` recipe visible to Conan.
`conan install` resolves the dependency graph and writes the CMake toolchain.
`pip install .` builds the extension module with that toolchain.

`--build=pycanha-core/*` forces a rebuild of the local `pycanha-core` package
from source instead of reusing a cached binary of an earlier revision.

macOS builds without MKL. Pass
`-o pycanha-core*:PYCANHA_OPTION_USE_MKL=False` in the Conan step.

Run the test suite to check the result:

```bash
pytest
```
