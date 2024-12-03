//
// Created by kevidgel on 11/6/24.
//

#pragma once

#ifndef TRIMESH_HPP
#define TRIMESH_HPP

#include <cuda_runtime.h>
#include <owl/common/math/AffineSpace.h>

namespace Geometry {
    using namespace owl;

    struct TriangleMesh {
        // Geometry
        vec3f* vertices;
        vec3f* normals;
        vec2f* tex_coords;
        vec3ui* indices;
        vec3ui* normal_indices;
        vec3ui* texcoord_indices;

        // Material
        uint material_id;
        bool has_tex;
        cudaTextureObject_t tex;
    };
}

#endif //TRIMESH_HPP
