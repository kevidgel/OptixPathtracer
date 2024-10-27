# Add source files
file(GLOB SOURCES
        "Main.cpp"
        "RenderBase.cpp"
        "ShaderProgram.cpp"
        "TraceHost.cpp"
)
file(GLOB HEADERS
        "RenderBase.hpp"
        "ShaderProgram.hpp"
        "TraceHost.hpp"
)
file(GLOB TYPE_HEADERS
        "types/geometry/*.hpp"
        "types/materials/*.hpp"
        "types/*.hpp"
)

# Provides SHADER_SOURCES
add_subdirectory(shaders)

message(STATUS "SOURCES: ${SHADER_SOURCES}")

# Add executable
add_executable(${PROJECT_NAME}
        ${SOURCES}
)

# Prebuild
add_dependencies(${PROJECT_NAME}
        EmbedTracePtx
)

set_target_properties(${PROJECT_NAME}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Include directories
target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/types
            ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
            EmbeddedShaders
            fmt::fmt
            spdlog::spdlog
            glad::glad
            glfw
            imgui::imgui
            Freetype::Freetype
            Fontconfig::Fontconfig
            TBB::tbb
            TBB::tbbmalloc
            owl::owl
)
