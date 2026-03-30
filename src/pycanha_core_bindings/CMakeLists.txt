
cmake_minimum_required(VERSION 3.25)

# Set the C++ standard globally
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(nanobind_template)

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
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/__init__.py
        ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/__init__.pyi
        ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/gmm.pyi
        ${CMAKE_CURRENT_SOURCE_DIR}/../pycanha_core/tmm.pyi
    DESTINATION
        pycanha_core
)

