cmake_minimum_required(VERSION 3.25)
project(OptixPathtracer)

set(CMAKE_CXX_STANDARD 20)

# Set preproccessor definitions
#add_definitions(-D__CUDA_ARCH__)

# Add packages (use vcpkg)
find_package(TBB CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Stb REQUIRED)

add_subdirectory(src)
add_subdirectory(deps)