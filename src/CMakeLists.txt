# Add source files
file(GLOB SOURCES
        "Main.cpp"
        "RenderBase.cpp"
        "Shader.cpp"
        "TraceHost.cpp"
        "loaders/*.cpp"
)
file(GLOB HEADERS
        "RenderBase.hpp"
        "Shader.hpp"
        "TraceHost.hpp"
        "loaders/*.hpp"
)
file(GLOB TYPE_HEADERS
        "shared_types/geometry/*.hpp"
        "shared_types/materials/*.hpp"
        "shared_types/*.hpp"
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
            ${CMAKE_CURRENT_SOURCE_DIR}/shared_types
            ${CMAKE_CURRENT_BINARY_DIR}/generated
            ${Stb_INCLUDE_DIR}
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
