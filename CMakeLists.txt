cmake_minimum_required(VERSION 2.8)
project(chill)
set(CHILL_VERSION 0.2.1) # 2.8 doesn't support project version clause

if(NOT DEFINED ROSEHOME)
    message( FATAL_ERROR "ROSEHOME is not set, try use -DROSEHOME" )
endif()
if (NOT DEFINED BOOSTHOME)
    message( FATAL_ERROR "BOOSTHOME is not set, try use -DBOOSTHOME" )
endif()

find_package(PythonLibs 2.7 REQUIRED)
find_package(BISON)
find_package(FLEX)

FLEX_TARGET(ExprScanner src/parse_expr.ll ${CMAKE_CURRENT_BINARY_DIR}/parse_expr.yy.cc COMPILE_FLAGS
    "--header-file=${CMAKE_CURRENT_BINARY_DIR}/parse_expr.ll.hh") # Hack to avoid generating header in root
BISON_TARGET(ExprParser src/parse_expr.yy ${CMAKE_CURRENT_BINARY_DIR}/parse_expr.tab.cc COMPILE_FLAGS "-t -d")
ADD_FLEX_BISON_DEPENDENCY(ExprScanner ExprParser)

string(TIMESTAMP build_date "\\\"%m/%d/%Y\\\"")

set(CORE_LIBS 
    m rose rt util omega codegen rosecg dl 
    boost_date_time boost_filesystem boost_program_options
    boost_regex boost_system boost_wave boost_iostreams)

set(CORE_SRC
    src/dep.cc
    src/irtools.cc
    src/loop.cc
    src/loop_basic.cc
    src/loop_datacopy.cc
    src/loop_extra.cc
    src/loop_tile.cc
    src/loop_unroll.cc
    src/omegatools.cc
    )

set(IR_CHILL_SRC
    src/ir_rose.cc
    src/ir_rose_utils.cc
    )

set(PYTHON_SRC
    src/chill_run.cc
    src/chill_run_util.cc
    src/chillmodule.cc
    ${FLEX_ExprScanner_OUTPUTS}
    ${BISON_ExprParser_OUTPUTS}
    )

set(COMMON_FLAGS "-DCHILL_BUILD_DATE=\"${build_date}\" -DCHILL_BUILD_VERSION=\"\\\"${CHILL_VERSION}\\\"\"")

set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${CMAKE_CXX_FLAGS}")

if (DEFINED OMEGAHOME)
    link_directories(${OMEGAHOME}/lib)
    set(OMEGA_INC ${OMEGAHOME}/include)
else()
    set(OMEGA_INC 
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/omega/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/codegen/include
        )
endif()

link_directories(${ROSEHOME}/lib ${BOOSTOME}/lib)

include_directories(
    include
    lib/rosecg/include
    ${OMEGA_INC}
    ${ROSEHOME}/include
    ${ROSEHOME}/include/rose
    ${BOOSTHOME}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PYTHON_INCLUDE_DIRS})

add_executable(chill ${CORE_SRC} ${PYTHON_SRC} ${IR_CHILL_SRC})
target_link_libraries(chill ${CORE_LIBS} ${PYTHON_LIBRARY})
add_dependencies(chill omega codegen rosecg)

install(TARGETS chill
        RUNTIME DESTINATION bin)

add_subdirectory(lib/omega)
add_subdirectory(lib/codegen)
add_subdirectory(lib/rosecg)

add_subdirectory(doc)
