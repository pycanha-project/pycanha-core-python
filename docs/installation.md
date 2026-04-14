# Installation

## Using `pip`
Just run `pip install pycanha-core`

## Building from source

Keep `pycanha-core` and `pycanha-core-python` side by side:

```text
pycanha-project/
├── pycanha-core/
└── pycanha-core-python/
```

Assuming your environment already satisfies the project prerequisites
(Python 3.13+, Conan, and a supported compiler), the shortest reliable build
flow is:

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
CC=/usr/bin/gcc-14 CXX=/usr/bin/g++-14 pip install . \
  --config-settings=cmake.define.CMAKE_TOOLCHAIN_FILE="$PWD/build/conan-deps/conan_toolchain.cmake"
```

The first command makes the local `pycanha-core` recipe available to Conan.
The `conan install` step then generates the dependency graph and the CMake
toolchain used by the bindings build. The final `pip install .` command builds
the extension module with that generated toolchain.

On Linux, the bundled Conan profile expects GCC 14. On macOS, use
`-o pycanha-core*:PYCANHA_OPTION_USE_MKL=False` in the Conan step and
`CC=clang CXX=clang++` in the `pip install` step.

The `--build=pycanha-core/*` flag is included on purpose: it forces Conan to
rebuild the local `pycanha-core` package from source instead of reusing an old
cached binary.

To verify the build afterwards, run:

```bash
pytest
```
