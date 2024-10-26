//
// Created by kevidgel on 10/22/24.
//

#include "TraceHost.hpp"
#include "shaders/Trace.cuh"

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <fstream>
#include <owl/owl.h>
#include <optional>
#include <vector>
#include <spdlog/spdlog.h>

#include "ShaderProgram.hpp"
#include "geometry/Sphere.hpp"

const int NUM_VERTICES = 8;
vec3f vertices[NUM_VERTICES] =
  {
    { -1.f,-1.f,-1.f },
    { +1.f,-1.f,-1.f },
    { -1.f,+1.f,-1.f },
    { +1.f,+1.f,-1.f },
    { -1.f,-1.f,+1.f },
    { +1.f,-1.f,+1.f },
    { -1.f,+1.f,+1.f },
    { +1.f,+1.f,+1.f }
  };

const int NUM_INDICES = 12;
vec3i indices[NUM_INDICES] =
  {
    { 0,1,3 }, { 2,3,0 },
    { 5,7,6 }, { 5,6,4 },
    { 0,4,5 }, { 0,5,1 },
    { 2,3,7 }, { 2,7,6 },
    { 1,5,7 }, { 1,7,3 },
    { 4,0,2 }, { 4,2,6 }
  };

const char *vertex_shader_source = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

const char *fragment_shader_source = R"(
#version 460 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform float num_samples;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    FragColor = texColor / num_samples;
}
)";

float screen_vertices[] = {
    // positions          // colors           // texture coords
     1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
};

unsigned int screen_indices[] = {
    0, 1, 2,   // first triangle
    0, 2, 3    // second triangle
};

std::optional<std::vector<char>> load_ptx_shader(const char* file_path) {
    std::ifstream file(file_path, std::ios::ate);
    if(!file.is_open()) {
        return std::nullopt;
    }

    size_t file_size = (size_t) file.tellg() + 1;
    if (file_size == 0) {
        return std::nullopt;
    };
    std::vector<char> file_contents(file_size,0);
    file.seekg(0);
    file.read(file_contents.data(), file_size);
    file.close();

    return std::make_optional(file_contents);
}

TraceHost::TraceHost(const Config& config)
    : config(config) {}

TraceHost::~TraceHost() {
    if (!initialized) return;
    /********** Cleanup OWL **********/
    owlModuleRelease(owl.module);
    owlRayGenRelease(owl.ray_gen);
    owlContextDestroy(owl.ctx);

    /********** Cleanup GL **********/
    glDeleteVertexArrays(1, &gl.vao);
    glDeleteBuffers(1, &gl.vbo);
    glDeleteBuffers(1, &gl.ebo);
    glDeleteBuffers(1, &gl.pbo);
    glDeleteTextures(1, &gl.disp_tex);
    delete gl.shader;
}

void TraceHost::init() {
    // Start time
    prev_time = std::chrono::high_resolution_clock::now();

    /********** Generate Scene **********/
    spdlog::info("Generating scene...");
    // Generate spheres
    using namespace Geometry;
    using namespace Material;

    std::vector<LambertianSphere> spheres;
    spheres.push_back({Sphere{vec3f(0.f, -1000.f, -1.f), 1000.f},
                       Lambertian{vec3f(0.2f, 0.2f, 0.2f)}});

    for (int i = -11; i < 11; i++) {
        for (int b = -11; b < 11; b++) {
            vec3f center(i + rnd(), 0.2f, b + rnd());
            spheres.push_back({
                Sphere{center, 0.2f},
                Lambertian{rnd3f()*rnd3f()}
            });
        }
    }

    spheres.push_back({
        Sphere{vec3f(0.f, 1.f, 0.f), 1.f},
        Lambertian{vec3f(0.8f, 0.3f, 0.3f)}
    });
    spheres.push_back({
        Sphere{vec3f(-4.f, 1.f, 0.f), 1.f},
        Lambertian{vec3f(0.8f, 0.3f, 0.3f)}
    });
    spheres.push_back({
        Sphere{vec3f(4.f, 1.f, 0.f), 1.f},
        Lambertian{vec3f(0.7f, 0.6f, 0.5f)}
    });

    /********** Initialize GL **********/
    spdlog::info("Initializing shaders...");

    gl.shader = new ShaderProgram(vertex_shader_source, fragment_shader_source);

    // Generate buffers
    glGenVertexArrays(1, &gl.vao);
    glGenBuffers(1, &gl.vbo);
    glGenBuffers(1, &gl.ebo);

    glBindVertexArray(gl.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(screen_vertices),
        screen_vertices,
        GL_STATIC_DRAW
        );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(screen_indices),
        screen_indices,
        GL_STATIC_DRAW
        );

    // Position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void*)0
        );
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void*) (3 * sizeof(float))
        );
    glEnableVertexAttribArray(1);
    // Texcoords
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        (void*) (6 * sizeof(float))
        );
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create pbo
    glGenBuffers(1, &gl.pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl.pbo);

    // Initialize buffer data
    glBufferData(GL_PIXEL_UNPACK_BUFFER, config.width * config.height * sizeof(float4), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Generate textures
    glGenTextures(1, &gl.disp_tex);
    glBindTexture(GL_TEXTURE_2D, gl.disp_tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, config.width, config.height, 0, GL_RGBA, GL_FLOAT, nullptr);

    gl.shader->use();
    gl.shader->set_int("texture1", 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /********** Initialize OWL **********/
    spdlog::info("Initializing OWL...");

    // Create context + module
    owl.ctx = owlContextCreate(nullptr, 1);

    auto ptx_file_res = load_ptx_shader(config.ptx_source);
    if (!ptx_file_res.has_value()) {
        throw std::runtime_error("Failed to load PTX file");
    }
    std::vector<char> ptx_file = ptx_file_res.value();
    owl.module = owlModuleCreate(owl.ctx, ptx_file.data());

    // Set sphere type
    OWLVarDecl lambertian_sphere_geom_vars[] = {
        { "prims", OWL_BUFPTR, OWL_OFFSETOF(LambertianSpheresGeom, prims) },
        { nullptr }
    };

    OWLGeomType lambertian_sphere_geom_type = owlGeomTypeCreate(
        owl.ctx,
        OWL_GEOMETRY_USER,
        sizeof(LambertianSpheresGeom),
        lambertian_sphere_geom_vars,
        -1
    );

    owlGeomTypeSetClosestHit(lambertian_sphere_geom_type, 0, owl.module, "LambertianSpheres");
    owlGeomTypeSetIntersectProg(lambertian_sphere_geom_type, 0, owl.module, "LambertianSpheres");
    owlGeomTypeSetBoundsProg(lambertian_sphere_geom_type, owl.module, "LambertianSpheres");

    owlBuildPrograms(owl.ctx);
    // // Create tri mesh type
    // OWLVarDecl triangle_geom_vars[] = {
    //     {"index", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData, index)},
    //     {"vertex", OWL_BUFPTR, OWL_OFFSETOF(TrianglesGeomData, vertex)},
    //     {"color", OWL_FLOAT3, OWL_OFFSETOF(TrianglesGeomData, color)},
    // };
    //
    // OWLGeomType triangles_geom_type = owlGeomTypeCreate(
    //     owl.ctx,
    //     OWL_GEOMETRY_TRIANGLES,
    //     sizeof(TrianglesGeomData),
    //     triangle_geom_vars,
    //     3
    //     );
    //
    // owlGeomTypeSetClosestHit(
    //     triangles_geom_type,
    //     0,
    //     owl.module,
    //     "TriangleMesh"
    //     );

    // Setup input buffers
    OWLBuffer lambertian_spheres_buffer = owlDeviceBufferCreate(owl.ctx, OWL_USER_TYPE(spheres[0]), spheres.size(), spheres.data());
    OWLGeom lambertian_spheres_geom = owlGeomCreate(owl.ctx, lambertian_sphere_geom_type);
    owlGeomSetPrimCount(lambertian_spheres_geom, spheres.size());
    owlGeomSetBuffer(lambertian_spheres_geom, "prims", lambertian_spheres_buffer);
    // OWLBuffer vertex_buffer = owlDeviceBufferCreate(owl.ctx, OWL_FLOAT3, NUM_VERTICES, vertices);
    // OWLBuffer index_buffer = owlDeviceBufferCreate(owl.ctx, OWL_INT3, NUM_INDICES, indices);
    //
    // OWLGeom triangles_geom = owlGeomCreate(owl.ctx, triangles_geom_type);
    // owlTrianglesSetVertices(triangles_geom, vertex_buffer, NUM_VERTICES, sizeof(vec3f), 0);
    // owlTrianglesSetIndices(triangles_geom, index_buffer, NUM_INDICES, sizeof(vec3i), 0);
    //
    // owlGeomSetBuffer(triangles_geom, "vertex", vertex_buffer);
    // owlGeomSetBuffer(triangles_geom, "index", index_buffer);
    // owlGeomSet3f(triangles_geom, "color", 1.f, 0.f, 0.f);

    // Setup accel structures
    // OWLGroup triangles_group = owlTrianglesGeomGroupCreate(owl.ctx, 1, &triangles_geom);
    // owlGroupBuildAccel(triangles_group);
    OWLGeom user_geoms[] = {
        lambertian_spheres_geom,
    };
    OWLGroup spheres_group = owlUserGeomGroupCreate(owl.ctx, 1, user_geoms);
    owlGroupBuildAccel(spheres_group);

    OWLGroup world = owlInstanceGroupCreate(owl.ctx, 1, &spheres_group);
    owlGroupBuildAccel(world);

    // Create miss program
    OWLVarDecl miss_prog_vars[] = {
        {"bg_color", OWL_FLOAT3, OWL_OFFSETOF(MissProgData, bg_color)},
        { nullptr }
    };

    OWLMissProg miss_prog = owlMissProgCreate(
        owl.ctx,
        owl.module,
        "Miss",
        sizeof(MissProgData),
        miss_prog_vars,
        -1
    );

    owlMissProgSet3f(miss_prog, "bg_color", 0.1f, 0.1f, 0.6f);

    // Create ray generation program
    OWLVarDecl ray_gen_vars[] = {
        {"pbo_ptr", OWL_RAW_POINTER, OWL_OFFSETOF(RayGenData, pbo_ptr)},
        {"pbo_size", OWL_INT2, OWL_OFFSETOF(RayGenData, pbo_size)},
        {"world", OWL_GROUP, OWL_OFFSETOF(RayGenData, world)},
        {"launch", OWL_BUFPTR, OWL_OFFSETOF(RayGenData, launch)},
        { nullptr }
    };

    // Create camera uniform buffer
    state.launch_params_buffer = owlDeviceBufferCreate(owl.ctx, OWL_USER_TYPE(LaunchParams), 1, nullptr);

    owl.ray_gen = owlRayGenCreate(
        owl.ctx,
        owl.module,
        "RayGen",
        sizeof(RayGenData),
        ray_gen_vars,
        -1
        );

    // Register PBO with CUDA
    cudaError_t result = cudaGraphicsGLRegisterBuffer(&state.cuda_pbo, gl.pbo, cudaGraphicsMapFlagsNone);
    if (result != cudaSuccess) {
        throw std::runtime_error("Failed to register PBO with CUDA");
    }

    void *dev_ptr;
    size_t size;
    cudaGraphicsMapResources(1, &state.cuda_pbo, 0);
    cudaGraphicsResourceGetMappedPointer(&dev_ptr, &size, state.cuda_pbo);

    owlRayGenSetPointer(owl.ray_gen, "pbo_ptr", dev_ptr);
    owlRayGenSet2i(owl.ray_gen, "pbo_size", config.width, config.height);
    owlRayGenSetGroup(owl.ray_gen, "world", world);
    owlRayGenSetBuffer(owl.ray_gen, "launch", state.launch_params_buffer);
    update_launch_params();

    spdlog::info("Building programs, pipeline, and SBT");
    owlBuildPrograms(owl.ctx);
    owlBuildPipeline(owl.ctx);
    owlBuildSBT(owl.ctx);

    cudaGraphicsUnmapResources(1, &state.cuda_pbo, 0);

    /********** Setup Render State **********/

    state.aspect = 1.0f * config.width / config.height;
    state.camera.look_from = { 5.f, 5.f, 5.f };
    state.camera.look_at = { 0.f, 0.f, 0.f };
    state.camera.up = { 0.f, 1.f, 0.f };
    state.camera.cos_fov_y = 0.66f;

    initialized = true;
}

std::pair<int, int> TraceHost::get_size() {
    return { config.width, config.height };
}

void TraceHost::resize_window(int width, int height) {
    state.aspect = 1.0f * width / height;
}

void TraceHost::update_launch_params() {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto delta = current_time - prev_time;
    prev_time = current_time;

    // Reflect host camera state
    vec3f camera_pos = state.camera.look_from;
    vec3f camera_d00 = normalize(state.camera.look_at - camera_pos);
    vec3f camera_ddu = state.camera.cos_fov_y * state.aspect * normalize(cross(camera_d00, state.camera.up));
    vec3f camera_ddv = state.camera.cos_fov_y * normalize(cross(camera_ddu, camera_d00));
    camera_d00 -= 0.5f * camera_ddu;
    camera_d00 -= 0.5f * camera_ddv;

    LaunchParams::Camera camera = {
        camera_pos,
        camera_d00,
        camera_ddu,
        camera_ddv
    };
    state.launch_params.set_camera(camera);

    // Increment frame count
    if (state.launch_params.dirty) {
        state.launch_params.frame.id = 1;
    }
    else {
        state.launch_params.frame.id++;
    }
    owlBufferUpload(state.launch_params_buffer, &state.launch_params, 0);
    state.launch_params.dirty = false;
}

void TraceHost::launch() {
    update_launch_params();
    owlRayGenLaunch2D(owl.ray_gen, config.width, config.height);
    cudaDeviceSynchronize();
}

void TraceHost::gl_draw() {
    gl.shader->use();
    launch();

    gl.shader->set_float("num_samples", static_cast<float>(state.launch_params.frame.id));

    // 1. Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl.disp_tex);
    // 2. Bind the PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl.pbo);
    // 3. Update the texture with PBO
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  config.width, config.height, GL_RGBA, GL_FLOAT, nullptr);

    // 4. Draw the screen quad
    glBindVertexArray(gl.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 5. Cleanup
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
