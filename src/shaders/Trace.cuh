#pragma once

#include <owl/owl.h>
#include <owl/common/math/vec.h>

using namespace owl;

struct TrianglesGeomData {
    /*! index buffer */
    vec3i* index;
    /*! vertex buffer */
    vec3f* vertex;
    /*! base color */
    vec3f color;
};

struct CameraData {
    vec3f pos;
    vec3f dir_00;
    vec3f dir_du;
    vec3f dir_dv;
};

struct RayGenData {
    /*! pixel buffer (created in gl context) */
    float4 *pbo_ptr;
    /*! dims of pixel buffer */
    vec2i  pbo_size;
    /*! world handle */
    OptixTraversableHandle world;
    /*! camera data in ubo */
    CameraData* camera_data;
};

struct MissProgData {
    /*! background color */
    vec3f bg_color;
};