
cmake_minimum_required(VERSION 3.25)

# Set the C++ standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pybind11_template)

find_package(pybind11 CONFIG REQUIRED)
find_package(pycanha-core CONFIG REQUIRED)

pybind11_add_module(pycanha_core MODULE
    pycanha-core-wrapper.cpp
)

target_link_libraries(pycanha_core PRIVATE pybind11::module pycanha-core::pycanha-core)
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

