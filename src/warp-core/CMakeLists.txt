set(WARP_CORE_SRC
    types/obj.c
    types/str.c
    buffers.c
    chunk.c
    common.c
    debug.c
    vm.c
    memory.c
    scanner.c
    diag.c
    compiler.c
    value.c
)
set(WARP_CORE_HDR
    include/warp/common.h
    include/warp/instr.h
    include/warp/obj.h
    include/warp/value.h
    include/warp/warp.h
    types/obj_impl.h
    buffers.h
    chunk.h
    debug.h
    memory.h
    diag_impl.h
    value_impl.h
    warp_internal.h
)

SET(ALL_SRC ${WARP_CORE_SRC} ${WARP_CORE_HDR})
LIST(SORT ALL_SRC)

add_library(warp-core STATIC ${ALL_SRC})

target_link_libraries(warp-core PUBLIC m unic termutils)
target_compile_options(warp-core PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_features(warp-core PUBLIC c_std_11)
target_include_directories(warp-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(warp-core
PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

