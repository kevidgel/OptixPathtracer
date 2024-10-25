//
// Created by kevidgel on 10/22/24.
//

#ifndef OPTIXPROGRAM_HPP
#define OPTIXPROGRAM_HPP

#include <chrono>

#include "shaders/Trace.cuh"

#include <owl/owl.h>
#include <optional>
#include <random>
#include <vector>

#include "ShaderProgram.hpp"

std::optional<std::vector<char>> load_ptx_shader(const char* file_path);

inline float rnd() {
    static std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(rng);
}

inline vec3f rnd3f() {
    return vec3f(rnd(), rnd(), rnd());
}

class TraceHost {
public:
    struct Config {
        const char* ptx_source;
        const int width;
        const int height;
    };

    TraceHost(const Config& config);
    ~TraceHost();

    void init();

    void resize_window(int width, int height);
    void set_camera();
    std::pair<int, int> get_size();
    void gl_draw();
    void launch();
private:
    /* Initial configuration */
    bool initialized = false;
    Config config;
    /* OpenGL state */
    struct GLState {
        // Shader program handle
        ShaderProgram* shader;
        // Vertex buffer object handle
        GLuint vbo;
        // Vertex array object handle
        GLuint vao;
        // Element buffer object handle
        GLuint ebo;
        // Pixel buffer handle
        GLuint pbo;
        // Display texture
        GLuint disp_tex;
    } gl;
    /* OWL for OptiX */
    struct OWLState {
        OWLContext ctx;
        OWLModule module;
        OWLRayGen ray_gen;
        OWLLaunchParams launch_params;
    } owl;
    /* State of the pathtracer */
    struct RenderState {
        /* Host definitions */
        struct Camera {
            vec3f look_from;
            vec3f look_at;
            vec3f up;
            float cos_fov_y;
        } camera;
        float aspect;

        /* Device definitions */
        OWLBuffer camera_uniform;
        cudaGraphicsResource* cuda_pbo;
    } state;
    std::chrono::high_resolution_clock::time_point start_time;
};



#endif //OPTIXPROGRAM_HPP
