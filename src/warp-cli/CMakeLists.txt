add_executable(warp cli.c)
target_link_libraries(warp PRIVATE warp-core termutils)

add_custom_target(run
    COMMAND warp
    DEPENDS warp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(dbg
    COMMAND lldb $<TARGET_FILE:warp>
    DEPENDS warp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)