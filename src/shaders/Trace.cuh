#pragma once

#include <owl/owl.h>
#include <owl/common/math/vec.h>
#include <cuda_runtime.h>

using namespace owl;

/**
* @brief Launch parameters
* @details This struct holds the camera parameters and frame information.
* Please use setters on the host side to modify the camera parameters.
*/
struct LaunchParams {
    bool dirty = false;
    int spp = 1; // samples per pixel

    struct Camera {
        vec3f pos;
        vec3f dir_00;
        vec3f dir_du;
        vec3f dir_dv;
    } camera;

    struct Frame {
        int id = 0;
        int accum_frames = 0;
    } frame;

    void set_camera(const Camera& next) {
        if (next.pos != camera.pos ||
            next.dir_00 != camera.dir_00 ||
            next.dir_du != camera.dir_du ||
            next.dir_dv != camera.dir_dv) {
            dirty = true;
            camera = next;
        }
    }
};

struct RayGenData {
    /*! pixel buffer (created in gl context) */
    vec4f* pbo_ptr;
    /*! dims of pixel buffer */
    vec2i pbo_size;
    /*! world handle */
    OptixTraversableHandle world;
    /*! env map alias table */
    struct EnvMapAliasTable {
        float* pdf;
        float* alias_pdf;
        int* alias_i;
        uint2 size;
    } env;
    /*! launch parameters in ubo */
    LaunchParams* launch;
};

struct MissProgData {
    /*! env_map */
    cudaTextureObject_t env_map;
};