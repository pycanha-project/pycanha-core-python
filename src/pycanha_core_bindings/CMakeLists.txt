
cmake_minimum_required(VERSION 3.25)

project(pycanha_core_bindings LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)
find_package(pycanha-core CONFIG REQUIRED)

nanobind_add_module(pycanha_core MODULE
    pycanha-core-wrapper.cpp
)

target_link_libraries(pycanha_core PRIVATE pycanha-core::pycanha-core)
set_target_properties(pycanha_core PROPERTIES
    BUILD_RPATH "\$ORIGIN/../../.."
    INSTALL_RPATH "\$ORIGIN/../../.."
)

install(TARGETS pycanha_core DESTINATION pycanha_core)

# Install runtime Python files (__init__.py, py.typed)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/
    DESTINATION pycanha_core
    FILES_MATCHING
    PATTERN "*.py"
    PATTERN "py.typed"
)

# Generate type stubs from the compiled extension module.
set(STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/stubs")

function(pycanha_add_stub target_name module_name output_file)
    if(WIN32)
        set(STUBGEN_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/pycanha_stubgen_wrapper.py")
        if(NOT EXISTS "${STUBGEN_WRAPPER}")
            file(WRITE "${STUBGEN_WRAPPER}" [=[
import argparse
import os
import sys


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--module", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--import-path", required=True)
    parser.add_argument("--marker", default=None)
    parser.add_argument("--dll-dir", action="append", default=[])
    args = parser.parse_args()

    for dll_dir in args.dll_dir:
        if dll_dir and os.path.isdir(dll_dir):
            try:
                os.add_dll_directory(dll_dir)
            except OSError:
                pass

    import nanobind.stubgen as stubgen

    argv = [
        "stubgen.py",
        "-q",
        "-i",
        args.import_path,
        "-m",
        args.module,
        "-o",
        args.output,
    ]

    if args.marker:
        argv.extend(["-M", args.marker])

    sys.argv = argv
    stubgen.main()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
]=])
        endif()

        get_filename_component(PYTHON_BIN_DIR "${Python_EXECUTABLE}" DIRECTORY)

        set(_outputs "${output_file}")
        set(_stubgen_args
            --module "${module_name}"
            --output "${output_file}"
            --import-path "$<TARGET_FILE_DIR:pycanha_core>"
            --dll-dir "$<TARGET_FILE_DIR:pycanha_core>"
            --dll-dir "${PYTHON_BIN_DIR}"
            --dll-dir "${PYTHON_BIN_DIR}/DLLs"
            --dll-dir "${PYTHON_BIN_DIR}/Library/bin"
            --dll-dir "${PYTHON_BIN_DIR}/Lib/site-packages/mkl"
            --dll-dir "${PYTHON_BIN_DIR}/Lib/site-packages/umf"
            --dll-dir "${PYTHON_BIN_DIR}/Lib/site-packages/tbb"
        )

        if(ARGC GREATER 3)
            list(APPEND _stubgen_args --marker "${ARGV3}")
            list(APPEND _outputs "${ARGV3}")
        endif()

        add_custom_command(
            OUTPUT ${_outputs}
            COMMAND "${Python_EXECUTABLE}" "${STUBGEN_WRAPPER}" ${_stubgen_args}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            DEPENDS pycanha_core "${STUBGEN_WRAPPER}"
            VERBATIM
        )

        add_custom_target(${target_name} ALL DEPENDS ${_outputs})
    else()
        if(ARGC GREATER 3)
            nanobind_add_stub(
                ${target_name}
                MODULE ${module_name}
                OUTPUT "${output_file}"
                PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
                DEPENDS pycanha_core
                MARKER_FILE "${ARGV3}"
            )
        else()
            nanobind_add_stub(
                ${target_name}
                MODULE ${module_name}
                OUTPUT "${output_file}"
                PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
                DEPENDS pycanha_core
            )
        endif()
    endif()
endfunction()

pycanha_add_stub(pycanha_core_init_stub pycanha_core "${STUB_DIR}/__init__.pyi" "${STUB_DIR}/py.typed")
pycanha_add_stub(pycanha_core_gmm_stub pycanha_core.gmm "${STUB_DIR}/gmm.pyi")
pycanha_add_stub(pycanha_core_tmm_stub pycanha_core.tmm "${STUB_DIR}/tmm.pyi")
pycanha_add_stub(pycanha_core_parameters_stub pycanha_core.parameters "${STUB_DIR}/parameters.pyi")
pycanha_add_stub(pycanha_core_solvers_stub pycanha_core.solvers "${STUB_DIR}/solvers.pyi")

# Install generated stubs
install(
    DIRECTORY "${STUB_DIR}/"
    DESTINATION pycanha_core
    FILES_MATCHING
    PATTERN "*.pyi"
    PATTERN "py.typed"
)

