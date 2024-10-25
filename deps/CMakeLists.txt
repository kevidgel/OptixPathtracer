set(OWL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/owl)
add_subdirectory(${OWL_DIR} EXCLUDE_FROM_ALL)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE owl::owl
)