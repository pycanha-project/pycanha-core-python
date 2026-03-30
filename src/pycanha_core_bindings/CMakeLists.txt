
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

# Generate type stubs from the compiled extension module
set(STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/stubs")

nanobind_add_stub(
    pycanha_core_init_stub
    MODULE pycanha_core
    OUTPUT "${STUB_DIR}/__init__.pyi"
    PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
    DEPENDS pycanha_core
    MARKER_FILE "${STUB_DIR}/py.typed"
)

nanobind_add_stub(
    pycanha_core_gmm_stub
    MODULE pycanha_core.gmm
    OUTPUT "${STUB_DIR}/gmm.pyi"
    PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
    DEPENDS pycanha_core
)

nanobind_add_stub(
    pycanha_core_tmm_stub
    MODULE pycanha_core.tmm
    OUTPUT "${STUB_DIR}/tmm.pyi"
    PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
    DEPENDS pycanha_core
)

nanobind_add_stub(
    pycanha_core_parameters_stub
    MODULE pycanha_core.parameters
    OUTPUT "${STUB_DIR}/parameters.pyi"
    PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
    DEPENDS pycanha_core
)

nanobind_add_stub(
    pycanha_core_solvers_stub
    MODULE pycanha_core.solvers
    OUTPUT "${STUB_DIR}/solvers.pyi"
    PYTHON_PATH "$<TARGET_FILE_DIR:pycanha_core>"
    DEPENDS pycanha_core
)

# Install generated stubs
install(
    DIRECTORY "${STUB_DIR}/"
    DESTINATION pycanha_core
    FILES_MATCHING
    PATTERN "*.pyi"
    PATTERN "py.typed"
)

