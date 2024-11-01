add_subdirectory(owl EXCLUDE_FROM_ALL)
add_subdirectory(rapidobj)
add_subdirectory(argparse)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
            owl::owl
            rapidobj::rapidobj
            argparse
)