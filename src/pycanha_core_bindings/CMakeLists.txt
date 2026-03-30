
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

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/
    DESTINATION pycanha_core
    FILES_MATCHING
    PATTERN "*.py"
    PATTERN "*.pyi"
    PATTERN "py.typed"
)

