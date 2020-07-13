cmake_minimum_required(VERSION 2.8)
project(Svar)
include(cmake/PICMake.cmake)

set(CMAKE_CXX_STANDARD 11)

option(BUILD_PYTHON "build svarpy module to support call python from c++" ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

pi_git_hash(GIT_VERSION)
add_definitions(-DBUILD_VERSION=\"${GIT_VERSION}\")

include_directories(src src/Svar src/cpp/GSLAM/core)

pi_add_target(svar BIN src/cpp REQUIRED System LTO)
pi_add_target(svar_cbor SHARED src/modules/svar_cbor REQUIRED System LTO)
pi_add_target(sample_module SHARED src/modules/sample_module REQUIRED System LTO)

if(BUILD_PYTHON)
  pi_add_target(svarpy SHARED src/python/svarpy/src REQUIRED Python System LTO)
endif()

pi_report_targets()
pi_install(TARGETS svar sample_module svar_cbor svarpy HEADERS src/Svar CMAKE "${PROJECT_SOURCE_DIR}/cmake/FindSvar.cmake.in")

#####################################################################
# Add Tab Completion Support
if(UNIX)
    install(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/cmake/tab_completion.cmake)
endif()
