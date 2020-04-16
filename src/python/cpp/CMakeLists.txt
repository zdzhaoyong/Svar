cmake_minimum_required(VERSION 2.8.12)
project(sample_call_python)

set(CMAKE_CXX_STANDARD 11)

include(../../../cmake/PICMake.cmake)
include_directories(../..)

pi_add_target(sample_call_python BIN main.cpp REQUIRED System LTO)
pi_report_target()
