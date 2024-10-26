//
// Created by kevidgel on 10/20/24.
//
#define GLFW_INCLUDE_NONE

#include "RenderBase.hpp"
#include "shaders/Trace.cuh"

#include <fstream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <optional>
#include <fontconfig/fontconfig.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void GLAPIENTRY debug_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
    )
{
    if (type != GL_DEBUG_TYPE_ERROR) {
        spdlog::debug("GL CALLBACK type = 0x{} severity = 0x{}, message = {}\n",
            type,
            severity,
            message);
    }
    else {
        spdlog::error("GL CALLBACK type = 0x{} severity = 0x{}, message = {}\n",
            type,
            severity,
            message);
    }
}

std::optional<std::string> get_default_font() {
    FcConfig* config = FcInitLoadConfigAndFonts();
    FcPattern* pattern = FcPatternCreate();
    FcObjectSet* object_set = FcObjectSetBuild(FC_FILE, nullptr);
    FcFontSet* font_set = FcFontList(config, pattern, object_set);

    std::string font_path;
    if (font_set && font_set->nfont > 0) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(font_set->fonts[0], FC_FILE, 0, &file) == FcResultMatch) {
            font_path = reinterpret_cast<const char*>(file);
        }
        else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    FcFontSetDestroy(font_set);
    FcObjectSetDestroy(object_set);
    FcPatternDestroy(pattern);
    FcConfigDestroy(config);

    return font_path;
}

RenderBase::RenderBase(const Config& config)
    : config(config) {}

RenderBase::RenderBase() {}

RenderBase::~RenderBase() {
}

void RenderBase::run() {
    init_glfw();
    init_programs();
    init_imgui();
    render_loop();
    destroy();
}

void RenderBase::init_glfw() {
    spdlog::info("Initializing GLFW");
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    state.window = glfwCreateWindow(
        config.window_width,
        config.window_height,
        "Pathtracing with OptiX",
        NULL,
        NULL
        );
    if (state.window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(state.window);

    // GLAD function pointer management
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSetWindowUserPointer(state.window, this);

    // Framebuffer callback
    glfwSetFramebufferSizeCallback(state.window, framebuffer_size_callback);

    // Window resize callback
    glfwSetWindowSizeCallback(state.window, [](GLFWwindow* window, int width, int height) {
        RenderBase* app = reinterpret_cast<RenderBase*>(glfwGetWindowUserPointer(window));
        app->resize_window(window, width, height);
    });

    // OpenGL info
    const GLubyte* gl_version = glGetString(GL_VERSION);
    spdlog::info("GL_VERSION: {}", reinterpret_cast<const char*>(gl_version));
    const GLubyte* gl_renderer = glGetString(GL_RENDERER);
    spdlog::info("GL_RENDERER: {}", reinterpret_cast<const char*>(gl_renderer));

    // Enable debug
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, nullptr);

    glEnable(GL_FRAMEBUFFER_SRGB);
}

void RenderBase::init_programs() {
    spdlog::info("Initializing device code...");
    optix = new TraceHost({
        "shaders/CMakeFiles/TracePtx.dir/Trace.ptx",
        config.window_width,
        config.window_height,
    });
    optix->init();
}

void RenderBase::init_imgui() {
    spdlog::info("Initializing ImGui...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(state.window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    auto font_path_res = get_default_font();
    if (font_path_res.has_value()) {
        std::string font_path = font_path_res.value();
        spdlog::debug("Using font: {}", font_path);
        ImFontConfig font_config;
        io.Fonts->AddFontFromFileTTF(font_path_res.value().c_str(), 16.0f, &font_config);
    }
    else {
        spdlog::warn("Could not find a default font, using the ImGui default font.");
    }
}


void RenderBase::process_input() {
    if (glfwGetKey(state.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(state.window, true);
    }
}

void RenderBase::resize_window(GLFWwindow* window, int width, int height) {
    if (optix) {
         optix->resize_window(width, height);
    }
}

void RenderBase::render_loop() {
    spdlog::info("Entering render loop...");
    while(!glfwWindowShouldClose(state.window)) {
        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::BeginMainMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(state.window, true);
            }
            ImGui::EndMenu();
        }

        auto fps = fmt::format("{:.1f} FPS", ImGui::GetIO().Framerate);
        float window_width = ImGui::GetWindowWidth();;
        float text_width = ImGui::CalcTextSize(fps.c_str()).x;
        ImGui::SameLine(window_width - 10 - text_width);
        ImGui::Text(fps.c_str());
        ImGui::EndMainMenuBar();

        // Input
        process_input();

        // Rendering
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        // Run optix texture program
        if (optix) {
           optix->gl_draw();
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(state.window);
        glfwPollEvents();
    }
}

void RenderBase::destroy() {
    spdlog::info("Cleaning up ImGui....");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    spdlog::info("Cleaning up OptiX program...");
    delete optix;

    glfwTerminate();
}