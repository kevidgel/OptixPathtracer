## Set the path to the slangc compiler
#if(DEFINED ENV{SLANG_PATH})
#    set(SLANGC_PATH "$ENV{SLANG_PATH}/bin/slangc")
#else()
#    message(FATAL_ERROR "SLANG_PATH environment variable is not set. Please set it to the path of your slang installation.")
#endif()
#
## Set the source and destination directories
#set(SHADER_SOURCE_DIR ".")
#set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src/shaders")
#
## Create the output directory
#file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
#
## Find all .slang files in the source directory
#file(GLOB SHADER_SOURCES "${SHADER_SOURCE_DIR}/*.slang")
#
## Create a custom target for shader compilation
#add_custom_target(compile_shaders)
#
#
## Iterate over each shader file
#foreach(SHADER_SOURCE ${SHADER_SOURCES})
#    # Get the filename without extension
#    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
#
#    # Set the output path for the compiled shader
#    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
#    set(SHADER_OUTPUT_C "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.c")
#
#    # Add custom command to compile the shader
#    add_custom_command(
#            OUTPUT ${SHADER_OUTPUT}
#            COMMAND ${SLANGC_PATH} ${SHADER_SOURCE} -profile glsl_460 -target spirv -o ${SHADER_OUTPUT} -entry main
#            DEPENDS ${SHADER_SOURCE}
#            COMMENT "Compiling shader ${SHADER_NAME}"
#    )
#
#    # Add custom command to generate C file for embedding
#    add_custom_command(
#            OUTPUT ${SHADER_OUTPUT_C}
#            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/embedfile ${SHADER_NAME} ${SHADER_OUTPUT}
#            DEPENDS ${SHADER_OUTPUT}
#            COMMENT "Generating C file for shader ${SHADER_NAME}"
#    )
#
#    # Add the output to the custom target
#    add_custom_target(${SHADER_NAME}_shader DEPENDS ${SHADER_OUTPUT} ${SHADER_OUTPUT_C})
#    add_dependencies(compile_shaders ${SHADER_NAME}_shader)
#endforeach()

# Executable for embedding
add_executable(EmbedShader EmbedShader.cpp)
set_target_properties(
        EmbedShader
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

# CUDA PTX compilation
enable_language(CUDA)
add_library(TracePtx OBJECT Trace.cu Trace.cuh)
target_link_libraries(
        TracePtx
        owl::owl
)
target_include_directories(
        TracePtx
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../types
)
set_target_properties(
        TracePtx
        PROPERTIES
        CUDA_PTX_COMPILATION ON
)

# Add custom target
add_custom_target(EmbedPtx DEPENDS EmbedShader TracePtx)

# Get ptx files
# NOTE: This is super hacky
file(GLOB_RECURSE PTX_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/*.ptx)
set(PTX_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/generated/src/shaders")
foreach(PTX_SOURCE ${PTX_SOURCES})
    get_filename_component(PTX_NAME ${PTX_SOURCE} NAME_WE)
    set(PTX_OUTPUT "${PTX_OUTPUT_DIR}/${PTX_NAME}.ptx.hpp")
    add_custom_command(
            OUTPUT ${PTX_OUTPUT}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/EmbedShader ${PTX_SOURCE} ${PTX_OUTPUT}
            DEPENDS ${PTX_SOURCE}
            COMMENT "Embedding PTX ${PTX_NAME}"
    )
    add_custom_target(EmbedPtx_${PTX_NAME} DEPENDS ${PTX_OUTPUT})
    add_dependencies(EmbedPtx EmbedPtx_${PTX_NAME})
endforeach()
