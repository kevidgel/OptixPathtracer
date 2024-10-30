add_subdirectory(owl EXCLUDE_FROM_ALL)
add_subdirectory(rapidobj)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
            owl::owl
            rapidobj::rapidobj
)