
cmake_minimum_required(VERSION 3.25)

# Set the C++ stanadard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pybind11_template)

find_package(pybind11 CONFIG REQUIRED)
find_package(pycanha-core CONFIG REQUIRED)


pybind11_add_module(pycanha_core MODULE 
    pycanha-core-wrapper.cpp
)

target_link_libraries(pycanha_core PRIVATE pycanha-core::pycanha-core)
target_compile_definitions(pycanha_core PRIVATE VERSION_INFO=${PROJECT_VERSION})

install(TARGETS pycanha_core DESTINATION pycanha_core)

