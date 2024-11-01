//
// Created by kevidgel on 10/20/24.
//

#pragma once

#ifndef RENDERBASE_HPP
#define RENDERBASE_HPP
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include "TraceHost.hpp"

class RenderBase {
public:
    struct Config {
        int window_width = 1280;
        int window_height = 720;
        std::optional<std::string> model;
        std::optional<std::string> env_map;
    } config;

    RenderBase(const Config& config);
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



#endif //RENDERBASE_HPP
