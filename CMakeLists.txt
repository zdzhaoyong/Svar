cmake_minimum_required(VERSION 2.8)
project(Svar)
include(cmake/PICMake.cmake)

set(CMAKE_CXX_FLAGS -std=c++11)
INCLUDE_DIRECTORIES(src/Svar)

pi_add_target(svar BIN src/cpp/svar REQUIRED System)
pi_add_target(sample_module SHARED src/sample_module REQUIRED System)
pi_add_target(sample_use BIN src/cpp/sample_use REQUIRED System)

pi_report_targets()
pi_install(TARGETS svar HEADERS src/Svar)
