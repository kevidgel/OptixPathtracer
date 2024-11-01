#pragma once

#include <owl/owl.h>
#include <owl/common/math/vec.h>
#include <cuda_runtime.h>

using namespace owl;

struct TrianglesGeomData {
    /*! index buffer */
    vec3i* index;
    /*! vertex buffer */
    vec3f* vertex;
    /*! base color */
    vec3f color;
};

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
    vec4f *pbo_ptr;
    /*! dims of pixel buffer */
    vec2i  pbo_size;
    /*! world handle */
    OptixTraversableHandle world;
    /*! launch parameters in ubo */
    LaunchParams* launch;
};

struct MissProgData {
    /*! env_map */
    cudaTextureObject_t env_map;
};