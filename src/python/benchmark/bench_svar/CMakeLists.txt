cmake_minimum_required(VERSION 2.8)

include(../../../../cmake/PICMake.cmake)

set(CMAKE_CXX_STANDARD 11)

include_directories(../../..)

set(Python_ADDITIONAL_VERSIONS 3.7 3.6 3.5 3.4)

pi_add_target(bench SHARED bench_svar.cpp REQUIRED Python System LTO)

set_target_properties(bench PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}")
set_target_properties(bench PROPERTIES SUFFIX "${PYTHON_MODULE_EXTENSION}")

pi_report_target()


