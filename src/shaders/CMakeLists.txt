# Executable for embedding
add_executable(EmbedShader
        EmbedShader.cpp
)
set_target_properties(EmbedShader
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Trace PTX compilation
enable_language(CUDA)
add_library(TracePtx
        OBJECT
            Trace.cu
            Trace.cuh
)
target_link_libraries(
        TracePtx
        owl::owl
)
target_include_directories(TracePtx
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../shared_types
)
set_target_properties(TracePtx
        PROPERTIES
            CUDA_PTX_COMPILATION ON
)
target_compile_options(TracePtx
        PRIVATE
            $<$<CONFIG:DEBUG>:-G>
            $<$<CONFIG:RELEASE>:--use_fast_math>
)

# Create directory for generated ptx
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated/")

# NOTE: This is super hacky
set(PTX_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/TracePtx.dir/Trace.ptx")
get_filename_component(PTX_NAME ${PTX_SOURCE} NAME_WE)
set(PTX_OUTPUT_DIR "${PROJECT_BINARY_DIR}/generated")
set(PTX_OUTPUT "${PTX_OUTPUT_DIR}/${PTX_NAME}.ptx.hpp")
add_custom_command(
        OUTPUT ${PTX_OUTPUT}
        COMMAND ${PROJECT_BINARY_DIR}/bin/EmbedShader ${PTX_SOURCE} ${PTX_OUTPUT}
        DEPENDS EmbedShader TracePtx Trace.cu Trace.cuh
        COMMENT "Embedding PTX ${PTX_NAME}"
)
message(STATUS "PTX output: ${PTX_OUTPUT}")

# Add custom target
add_custom_target(EmbedTracePtx
        DEPENDS
            ${PTX_OUTPUT}
)

add_library(EmbeddedShaders
        INTERFACE
            ${PTX_OUTPUT}
)

add_dependencies(EmbeddedShaders EmbedTracePtx)
target_include_directories(EmbeddedShaders
        INTERFACE
            ${CMAKE_BINARY_DIR}/generated
)
