# For a "regular" build with scikit-build-core, you would just use in the pyproject.toml:
# build-backend = "scikit_build_core.build"
# But here, we want to conan install the C++ dependencies before calling cmake, so there are three options:
# 1. To continue using scikit-build-core directly, but befor calling `pip install .` you would need to call `conan install .`
# 2. To integrate conan in CMake. This is still experimental for Conan 2.0. I might try it in the future.
# 3. To use a custom build backend that just do the conan install before leaving the rest to scikit-build-core, which is what we do here.

# Guard against direct execution of this file
if __name__ == "__main__":
    raise TypeError(
        "This module cannot be executed directly. It should be called by pip install."
    )


# Import the hooks from scikit-build-core that will be called by pip
from scikit_build_core.build import *
import subprocess

# Get version of the bindings from pyproject.toml
import toml

bindings_version_str = toml.load("pyproject.toml")["project"]["version"]

# Configure the binding version in conanfile.txt (of the bindings)
with open("src/pycanha_core/conanfile.txt.in", "r") as file:
    conanfile_txt_in = file.read()
    conanfile_txt = conanfile_txt_in.replace(
        "@CONFIG_PYCANHA_CORE_VERSION@", bindings_version_str
    )
    with open("src/pycanha_core/conanfile.txt", "w") as file:
        file.write(conanfile_txt)


# Check pycanha-core is available in ../pycanha-core. If available, check the version is correct.
# Read ../pycanha-core/conanfile.py
import re

try:
    with open("../pycanha-core/conanfile.py", "r") as file:
        conanfile_content = file.read()
        pycanha_core_version = re.search(
            r'version\s*=\s*"(\d+\.\d+\.\d+)"', conanfile_content
        )

        # Check binding version is the same as pycanha-core version
        if pycanha_core_version is None:
            raise RuntimeError(
                f"Could not find version in pycanha-core/conanfile.py. Required version is {bindings_version_str}"
            )
        if pycanha_core_version.group(1) != bindings_version_str:
            raise RuntimeError(
                f"Version mismatch. pycanha-core version is {pycanha_core_version.group(1)}, but bindings version is {bindings_version_str}"
            )

except FileNotFoundError:
    # Same as with conan create, if the package was already created, this step is not necessary.
    pass


result = subprocess.run(
    [
        "conan",
        "create",
        "../pycanha-core",
        "--build=missing",
        "-pr:h=src/pycanha_core/pycanha-core-conan-profile",
        "-pr:b=src/pycanha_core/pycanha-core-conan-profile",
    ],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)
if result.returncode == 0:
    print("Success!")
else:
    print("Error:", result.stderr.decode())
    # If the conan package was already created, this step is not necessary. When building
    # the wheel, the bindings code is copied elsewhere to for isolation, so conan create
    # will fail. But if it was executed before, conan install will work. So not raising error here.
    # raise RuntimeError("Conan install failed. See error above.")

# "Inject" the conan install before the build hooks are called.
result = subprocess.run(
    [
        "conan",
        "install",
        "src/pycanha_core",
        "--build=missing",
        "-pr:h=src/pycanha_core/pycanha-core-conan-profile",
        "-pr:b=src/pycanha_core/pycanha-core-conan-profile",
    ],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)
if result.returncode == 0:
    print("Success!")
else:
    print("Error:", result.stderr.decode())
    raise RuntimeError("Conan install failed. See error above.")


# MODIFY the CMakePresets file to make it works in linux and windows.
# This is a workaround. Probably it should be better configured in Conan with conanfile.txt or conanfile.py
import json

# Get the path of this script
import os

script_path = os.path.dirname(os.path.realpath(__file__))


with open(os.path.join(script_path, "CMakeUserPresets.json"), "r") as file:
    filedata = json.loads(file.read())
    # Read the include file with the Cmake presets
    cmake_preset_file_path = filedata["include"][0]

    # Read the CMake presets to replace some settings
    with open(cmake_preset_file_path, "r") as file:
        filedata = file.read()

        # Replace conan-default with conan-release for Windows
        filedata = filedata.replace("conan-default", "conan-release")

        # In linux with gcc, only Ninja works, but conan generates a preset with Unix Makefiles
        filedata = filedata.replace("Unix Makefiles", "Ninja")

    # Write the new CMake presets
    with open(cmake_preset_file_path, "w") as file:
        file.write(filedata)
