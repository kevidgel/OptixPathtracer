cmake_minimum_required(VERSION 3.25)
project(OptixPathtracer)

set(CMAKE_CXX_STANDARD 20)

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

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

add_dependencies(
        ${PROJECT_NAME}
        EmbedPtx
)