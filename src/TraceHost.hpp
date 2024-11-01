//
// Created by kevidgel on 10/22/24.
//

#pragma once

#ifndef TRACEHOST_HPP
#define TRACEHOST_HPP

#include <chrono>
#include <owl/owl.h>
#include <optional>
#include <random>
#include <vector>

#include "shaders/Trace.cuh"
#include "Shader.hpp"

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
        std::optional<std::string> model;
        std::optional<std::string> env_map;
        const int width;
        const int height;
    };

    enum CameraActions {
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        MoveForward,
        MoveBackward,
        RotateLeft,
        RotateRight,
        RotateUp,
        RotateDown
    };

    TraceHost(const Config& config);
    ~TraceHost();

    void init();

    void resize_window(int width, int height);
    void increment_camera(CameraActions action, float delta);
    void update_launch_params();
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
        Shader* shader;
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
        OWLMissProg miss_prog;
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
        OWLBuffer launch_params_buffer;
        LaunchParams launch_params;
        cudaGraphicsResource* cuda_pbo;
    } state;
    std::chrono::high_resolution_clock::time_point prev_time;
    std::chrono::duration<long, std::ratio<1, 1000000000>> approx_delta;
};

#endif // TRACEHOST_HPP
