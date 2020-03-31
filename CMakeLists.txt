cmake_minimum_required(VERSION 2.8)
project(Svar)
include(cmake/PICMake.cmake)

set(CMAKE_CXX_STANDARD 11)
INCLUDE_DIRECTORIES(src/Svar src/GSLAM/core)

pi_add_target(svar BIN src/cpp REQUIRED System)
pi_add_target(sample_module SHARED src/sample_module REQUIRED System LTO)

pi_report_targets()
pi_install(TARGETS svar sample_module HEADERS src/Svar)

#####################################################################
# Add Tab Completion Support
if(UNIX)
    install(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/cmake/tab_completion.cmake)
endif()
