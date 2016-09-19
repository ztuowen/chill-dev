find_package(BISON)
find_package(FLEX)

FLEX_TARGET(ExprScanner src/parseRel.ll ${CMAKE_CURRENT_BINARY_DIR}/parseRel.yy.cc COMPILE_FLAGS
    "--header-file=${CMAKE_CURRENT_BINARY_DIR}/parseRel.ll.hh") # Hack to avoid generating header in root
BISON_TARGET(ExprParser src/parseRel.yy ${CMAKE_CURRENT_BINARY_DIR}/parseRel.tab.cc COMPILE_FLAGS "-t -d")
ADD_FLEX_BISON_DEPENDENCY(ExprScanner ExprParser)

include_directories(
    include
    ${CMAKE_CURRENT_BINARY_DIR}
    )

add_library(parseRel
    src/parseRel.cc
    ${FLEX_ExprScanner_OUTPUTS}
    ${BISON_ExprParser_OUTPUTS}
    )
