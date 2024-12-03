/**
* @file ObjLoader.cpp
* @brief Implementation of the ObjLoader class for loading OBJ files.
*/

#include <spdlog/spdlog.h>

#include "ObjLoader.hpp"


ObjLoader::ObjLoader(const Config& config)
    : config(config) {}

ObjLoader::~ObjLoader() {
}

bool ObjLoader::load(const std::string& filename) {
    spdlog::info("ObjLoader: Loading OBJ file: {}", filename);
    obj::Result res = obj::ParseFile(filename);
    if (res.error) {
        spdlog::error("ObjLoader: Failed to load OBJ file: {}", res.error.code.message());
        return false;
    }

    // Triangulate it
    if (!Triangulate(res)) {
        spdlog::error("ObjLoader: Failed to triangulate OBJ file: {}", filename);
        return false;
    }

    size_t num_indices = 0;
    for (const auto& shape : res.shapes) {
        num_indices += shape.mesh.indices.size();
    }

    // Fill indices
    data.indices.resize(num_indices);
    data.normal_indices.resize(num_indices);
    data.texcoord_indices.resize(num_indices);
    size_t offset = 0;
    for (const auto& shape : res.shapes) {
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            data.normal_indices[offset] = vec3ui(
                shape.mesh.indices[i].normal_index,
                shape.mesh.indices[i + 1].normal_index,
                shape.mesh.indices[i + 2].normal_index
            );
            data.texcoord_indices[offset] = vec3ui(
                shape.mesh.indices[i].texcoord_index,
                shape.mesh.indices[i + 1].texcoord_index,
                shape.mesh.indices[i + 2].texcoord_index
            );
            data.indices[offset++] = vec3ui(
                shape.mesh.indices[i].position_index,
                shape.mesh.indices[i + 1].position_index,
                shape.mesh.indices[i + 2].position_index
            );
        }
    }

    // Fill vertices, normals, and texcoords
    if ((config.loadFlags & LoadFlags::Vertices) == LoadFlags::Vertices) {
        size_t n = res.attributes.positions.size();
        data.vertices.resize(n / 3);
        for (size_t i = 0; i < n; i += 3) {
            data.vertices[i / 3] = vec3f(
                res.attributes.positions[i],
                res.attributes.positions[i + 1],
                res.attributes.positions[i + 2]
            );
        }
    }

    if ((config.loadFlags & LoadFlags::Normals) == LoadFlags::Normals) {
        size_t n = res.attributes.normals.size();
        data.normals.resize(n / 3);
        for (size_t i = 0; i < n; i += 3) {
            data.normals[i / 3] = vec3f(
                res.attributes.normals[i],
                res.attributes.normals[i + 1],
                res.attributes.normals[i + 2]
            );
        }
    }

    // Fill vertices, normals, and texcoords
    if ((config.loadFlags & LoadFlags::TexCoords) == LoadFlags::TexCoords) {
        size_t n = res.attributes.texcoords.size();
        data.texcoords.resize(n / 2);
        for (size_t i = 0; i < n; i += 2) {
            data.texcoords[i / 2] = vec2f(
                res.attributes.texcoords[i],
                res.attributes.texcoords[i + 1]
            );
        }
    }

    return true;
}

void ObjLoader::clear() {
    // Clear any loaded data
    data.vertices.clear();
    data.indices.clear();
}