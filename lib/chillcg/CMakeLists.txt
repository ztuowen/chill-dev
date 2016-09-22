set(CODEGEN_CHILL_SRC
    src/CG_chillBuilder.cc
    src/CG_chillRepr.cc
    )

include_directories(
    include
    ${OMEGA_INC}
    )

add_library(chillcg ${CODEGEN_CHILL_SRC})

install(TARGETS chillcg
        DESTINATION lib)

install(DIRECTORY include/code_gen
    DESTINATION include)
