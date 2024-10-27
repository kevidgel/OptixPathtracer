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
file (GLOB TYPE_HEADERS
        "types/geometry/*.hpp"
        "types/materials/*.hpp"
        "types/*.hpp"
)
file(GLOB SHADER_SOURCES
        "${PROJECT_SOURCE_DIR}/generated/src/shaders/*.hpp"
)

# Add executable
add_executable(
        ${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${TYPE_HEADERS}
        ${SHADER_SOURCES}
)

# Include directories
target_include_directories(
        ${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/types
        PRIVATE ${PROJECT_SOURCE_DIR}/generated/src/shaders
)