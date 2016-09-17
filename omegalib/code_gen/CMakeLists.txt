set(CG_SRC
    src/codegen.cc
    src/CG.cc
    src/CG_stringBuilder.cc
    src/CG_utils.cc

    src/rose_attributes.cc
    src/CG_roseBuilder.cc
    src/CG_roseRepr.cc
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")

include_directories(
    include
    ${OMEGAROOT}/omega_lib/include
    ${ROSEHOME}/include/rose
    ${BOOSTHOME}/include
    )

add_library(codegen
    ${CG_SRC}
    )

add_dependencies(codegen omega)

install(TARGETS codegen
    ARCHIVE DESTINATION lib/static)
install(DIRECTORY include
    DESTINATION .)
