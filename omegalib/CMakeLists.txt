cmake_minimum_required(VERSION 2.8)
project(omegalib)

if(NOT DEFINED ROSEHOME)
    message( FATAL_ERROR "ROSEHOME is not set, try use -DROSEHOME" )
endif()
if (NOT DEFINED BOOSTHOME)
    message( FATAL_ERROR "BOOSTHOME is not set, try use -DBOOSTHOME" )
endif()

set(OMEGAROOT ${PROJECT_SOURCE_DIR})

add_subdirectory(omega_lib)
add_subdirectory(code_gen)
add_subdirectory(omega_calc)
