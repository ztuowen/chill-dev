find_package(PythonLibs REQUIRED)
find_package(BISON)
find_package(FLEX)

FLEX_TARGET(ExprScanner src/parse_expr.ll ${CMAKE_CURRENT_BINARY_DIR}/parse_expr.yy.cc COMPILE_FLAGS
    "--header-file=${CMAKE_CURRENT_BINARY_DIR}/parse_expr.ll.hh") # Hack to avoid generating header in root
BISON_TARGET(ExprParser src/parse_expr.yy ${CMAKE_CURRENT_BINARY_DIR}/parse_expr.tab.cc COMPILE_FLAGS "-t -d")
ADD_FLEX_BISON_DEPENDENCY(ExprScanner ExprParser)

string(TIMESTAMP build_date "\\\"%m/%d/%Y\\\"")

set(CORE_LIBS 
    m rose rt util omega codegen dl 
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

set(COMMON_FLAGS "-DBUILD_ROSE -DCHILL_BUILD_DATE=\"${build_date}\" -DCHILL_BUILD_VERSION=\"\\\"0.2.1\\\"\"")

set(CMAKE_CXX_FLAGS "-DPYTHON ${COMMON_FLAGS}")

link_directories(${ROSEHOME}/lib ${BOOSTOME}/lib)

include_directories(
    include
    ${PROJECT_SOURCE_DIR}/omegalib/include 
    ${ROSEHOME}/include
    ${ROSEHOME}/include/rose
    ${BOOSTHOME}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PYTHON_INCLUDE_DIRS})

add_executable(chill ${CORE_SRC} ${PYTHON_SRC} ${IR_CHILL_SRC})
target_link_libraries(chill ${CORE_LIBS} ${PYTHON_LIBRARY})
add_dependencies(chill omega codegen)
