set(CODEGEN_ROSE_SRC
    src/CG_roseBuilder.cc
    src/CG_roseRepr.cc
    src/rose_attributes.cc
    )

include_directories(
    include
    ${OMEGA_INC}
    ${ROSEHOME}/include
    ${ROSEHOME}/include/rose
    ${BOOSTHOME}/include
    )

add_library(rosecg ${CODEGEN_ROSE_SRC})

install(TARGETS rosecg
        DESTINATION lib)

install(DIRECTORY include/code_gen
    DESTINATION include)
