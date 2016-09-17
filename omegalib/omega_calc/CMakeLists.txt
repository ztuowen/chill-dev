find_package(BISON)
find_package(FLEX)

FLEX_TARGET(OCScanner src/parser.ll ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc COMPILE_FLAGS "-+")
BISON_TARGET(OCParser src/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc COMPILE_FLAGS "-t -d")
ADD_FLEX_BISON_DEPENDENCY(OCScanner OCParser)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    include
    ${OMEGAROOT}/omega_lib/include
    ${OMEGAROOT}/code_gen/include
    ${OMEGAROOT}/basic/include
    )

string(TIMESTAMP build_date "\\\"%m/%d/%Y\\\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_CODEGEN -DOMEGA_BUILD_DATE=\"${build_date}\"")

add_executable(omega_calc
    EXCLUDE_FROM_ALL
    src/AST.cc
    src/myflex.cc
    ${FLEX_OCScanner_OUTPUTS}
    ${BISON_OCParser_OUTPUTS}
    )

add_dependencies(omega_calc omega codegen)
target_link_libraries(omega_calc omega codegen)

install(TARGETS omega_calc
        RUNTIME DESTINATION bin)
