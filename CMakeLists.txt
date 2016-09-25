cmake_minimum_required(VERSION 2.8)
project(chill)
set(CHILL_VERSION 0.2.1) # 2.8 doesn't support project version clause

find_package(PythonLibs 2.7 REQUIRED)
find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})

string(TIMESTAMP build_date "\\\"%m/%d/%Y\\\"")

set(CORE_LIBS
        omega codegen chillcg parseRel)

set(CORE_SRC
        src/dep.cc
        src/irtools.cc
        src/transformations/loop.cc
        src/transformations/loop_basic.cc
        src/transformations/loop_datacopy.cc
        src/transformations/loop_extra.cc
        src/transformations/loop_tile.cc
        src/transformations/loop_unroll.cc
        src/omegatools.cc
        )

set(IR_CHILL_SRC
        src/ir_clang.cc
        src/chillASTs.cc
        )

set(PYTHON_SRC
        src/chillmodule.cc
        )

set(PRINTER_SRC
        src/printer/generic.cpp
        src/printer/dump.cpp
        src/printer/cfamily.cpp
        )

llvm_map_components_to_libnames(llvm_libs all)
set(clang_libs
        clangTooling
        clangFrontendTool
        clangFrontend
        clangDriver
        clangSerialization
        clangCodeGen
        clangParse
        clangSema
        clangStaticAnalyzerFrontend
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangAnalysis
        clangARCMigrate
        clangRewrite
        clangRewriteFrontend
        clangEdit
        clangAST
        clangLex
        clangBasic
        )

set(COMMON_FLAGS "-DCHILL_BUILD_DATE=\"${build_date}\" -DCHILL_BUILD_VERSION=\"\\\"${CHILL_VERSION}\\\"\"")

set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${CMAKE_CXX_FLAGS}")

if (DEFINED OMEGAHOME)
    link_directories(${OMEGAHOME}/lib)
    set(OMEGA_INC ${OMEGAHOME}/include)
else ()
    set(OMEGA_INC
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/omega/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/parserel/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/codegen/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/chillcg/include
            )
endif ()

include_directories(
        include
        lib/chillcg/include
        lib/iegenlib/src
        ${OMEGA_INC}
        ${LLVM_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS})

add_executable(chill
        src/chill.cc
        ${CORE_SRC}
        ${PYTHON_SRC}
        ${IR_CHILL_SRC}
        ${PRINTER_SRC})

target_link_libraries(chill
        ${CORE_LIBS}
        ${llvm_libs}
        ${clang_libs}
        ${PYTHON_LIBRARY})
add_dependencies(chill omega codegen chillcg parseRel)

install(TARGETS chill
        RUNTIME DESTINATION bin)

add_subdirectory(lib/omega)
add_subdirectory(lib/codegen)
add_subdirectory(lib/chillcg)
add_subdirectory(lib/parserel)

add_subdirectory(doc)
