//
// Created by kevidgel on 10/20/24.
//

#ifndef APP_HPP
#define APP_HPP

#include <memory>

#include "ShaderProgram.hpp"

#include <cuda_runtime.h>
#include <unordered_map>
#include <GLFW/glfw3.h>
#include <owl/owl.h>
#include <vector>

#include "TraceHost.hpp"

class RenderBase {
public:
    struct Config {
        const int window_width = 1280;
        const int window_height = 720;
    } config;

    RenderBase(const Config& config);
    RenderBase();
    ~RenderBase();

    void run();
private:
    struct State {
        GLFWwindow* window;
    } state;

    TraceHost* optix;

    void init_glfw();
    void init_programs();
    void init_imgui();
    void process_input();
    void render_loop();
    void resize_window(GLFWwindow* window, int width, int height);
    void destroy();
};



#endif //APP_HPP
