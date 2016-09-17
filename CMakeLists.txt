cmake_minimum_required(VERSION 2.8)
project(chill)
set(CHILL_VERSION 0.2.1) # 2.8 doesn't support project version clause

if(NOT DEFINED ROSEHOME)
    message( FATAL_ERROR "ROSEHOME is not set, try use -DROSEHOME" )
endif()
if (NOT DEFINED BOOSTHOME)
    message( FATAL_ERROR "BOOSTHOME is not set, try use -DBOOSTHOME" )
endif()

add_subdirectory(chill)
add_subdirectory(omegalib)
