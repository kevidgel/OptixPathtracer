cmake_minimum_required(VERSION 3.25)
project(renderer)

set(CMAKE_CXX_STANDARD 20)

# Set slang path
set(ENV{SLANG_PATH} /opt/slang)

# CUDA config
set(CMAKE_CUDA_ARCHITECTURES native)
set(CMAKE_CUDA_COMPILER /usr/local/cuda-12.6/bin/nvcc)

# OptiX config
set(OptiX_ROOT_DIR /opt/NVIDIA-OptiX-SDK-8.0.0-linux64-x86_64/)

# add_definitions(-D__CUDA_ARCH__)

# Add packages (use vcpkg)
find_package(TBB CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(Fontconfig REQUIRED)

add_subdirectory(src/shaders)
add_subdirectory(src)
add_subdirectory(deps)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE fmt::fmt
        PRIVATE spdlog::spdlog
        PRIVATE glad::glad
        PRIVATE glfw
        PRIVATE imgui::imgui
        PRIVATE Freetype::Freetype
        PRIVATE Fontconfig::Fontconfig
        PRIVATE TBB::tbb TBB::tbbmalloc
        PRIVATE owl::owl
        PRIVATE ${CUDA_LIBRARIES}
)

add_dependencies(
        ${PROJECT_NAME}
        TracePtx
)