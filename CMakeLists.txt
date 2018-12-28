cmake_minimum_required(VERSION 2.8)
include(PICMake)

set(CMAKE_CXX_FLAGS -std=c++11)
pi_add_target(svar BIN src REQUIRED System)
pi_report_targets()
