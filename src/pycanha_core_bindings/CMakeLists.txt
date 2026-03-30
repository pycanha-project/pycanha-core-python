
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
        get_filename_component(PYTHON_BIN_DIR "${Python_EXECUTABLE}" DIRECTORY)
        set(STUBGEN_PATH_ENTRIES
            "$<TARGET_FILE_DIR:pycanha_core>"
            "${PYTHON_BIN_DIR}"
            "${PYTHON_BIN_DIR}/DLLs"
            "${PYTHON_BIN_DIR}/Library/bin"
            "${PYTHON_BIN_DIR}/Lib/site-packages/mkl"
            "${PYTHON_BIN_DIR}/Lib/site-packages/umf"
            "${PYTHON_BIN_DIR}/Lib/site-packages/tbb"
        )

        list(REMOVE_DUPLICATES STUBGEN_PATH_ENTRIES)
        list(JOIN STUBGEN_PATH_ENTRIES ";" STUBGEN_PATH_PREFIX)
        set(STUBGEN_PATH "${STUBGEN_PATH_PREFIX};$ENV{PATH}")

        set(_outputs "${output_file}")
        set(_stubgen_args -q -i "$<TARGET_FILE_DIR:pycanha_core>" -m "${module_name}" -o "${output_file}")

        if(ARGC GREATER 3)
            list(APPEND _stubgen_args -M "${ARGV3}")
            list(APPEND _outputs "${ARGV3}")
        endif()

        add_custom_command(
            OUTPUT ${_outputs}
            COMMAND ${CMAKE_COMMAND} -E env
                    "PATH=${STUBGEN_PATH}"
                    "${Python_EXECUTABLE}" -m nanobind.stubgen ${_stubgen_args}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            DEPENDS pycanha_core
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

