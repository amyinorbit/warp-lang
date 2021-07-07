set(WARP_CORE_SRC buffers.c chunk.c common.c debug.c vm.c memory.c)
set(WARP_CORE_HDR buffers.h chunk.h debug.h memory.h warp_internal.h)

SET(ALL_SRC ${WARP_CORE_SRC} ${WARP_CORE_HDR})
LIST(SORT ALL_SRC)

add_library(warp-core STATIC ${ALL_SRC})

target_link_libraries(warp-core PUBLIC m unic)
target_compile_options(warp-core PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_features(warp-core PUBLIC c_std_11)
target_include_directories(warp-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(warp-core
PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

