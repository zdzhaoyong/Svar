cmake_minimum_required(VERSION 2.8.12)
project(svarpy)

set(CMAKE_CXX_STANDARD 11)

include(../../../cmake/PICMake.cmake)
include_directories(../..)


set(PIPING ON)
if(PIPING)
  pi_add_target(svar SHARED src REQUIRED Python System LTO)

  set_target_properties(svar PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}")
  set_target_properties(svar PROPERTIES SUFFIX "${PYTHON_MODULE_EXTENSION}")

else()
  pi_add_target(svarpy SHARED src REQUIRED Python System LTO)
  pi_install(TARGETS svarpy)
endif()

pi_report_target()


