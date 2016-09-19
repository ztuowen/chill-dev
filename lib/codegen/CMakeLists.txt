set(CG_SRC
    src/codegen.cc
    src/CG.cc
    src/CG_stringBuilder.cc
    src/CG_utils.cc
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")

include_directories(
    include
    ${OMEGAROOT}/include
    )

add_library(codegen
    ${CG_SRC}
    )

add_dependencies(codegen omega)

install(TARGETS codegen
    ARCHIVE DESTINATION lib)
install(DIRECTORY include
    DESTINATION .)
