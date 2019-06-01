cmake_minimum_required(VERSION 2.8.12)
project(svarpy)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/pybind11/tools")

include_directories(.. pybind11/include)
include(pybind11/tools/pybind11Tools.cmake)
pybind11_add_module(svar src/Svar.cpp)
