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

    if (!Triangulate(res)) {
        spdlog::error("ObjLoader: Failed to triangulate OBJ file: {}", filename);
        return false;
    }

    const size_t num_positions = res.attributes.positions.size();
    size_t num_indices = 0;
    for (const auto& shape : res.shapes) {
        num_indices += shape.mesh.indices.size();
    }

    // Resize for 3D vertices and indices;
    data.vertices.resize(num_positions / 3);
    data.vert_indices.resize(num_indices);

    if ((config.loadFlags & LoadFlags::Vertices) == LoadFlags::Vertices) {
        for (size_t i = 0; i < num_positions; i += 3) {
            data.vertices[i / 3] = vec3f(
                res.attributes.positions[i],
                res.attributes.positions[i + 1],
                res.attributes.positions[i + 2]
            );
        }

        size_t offset = 0;
        for (const auto& shape : res.shapes) {
            for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
                data.vert_indices[offset++] = vec3ui(
                    shape.mesh.indices[i].position_index,
                    shape.mesh.indices[i + 1].position_index,
                    shape.mesh.indices[i + 2].position_index
                );
            }
        }
    }

    return true; // Placeholder return value
}

bool ObjLoader::load_to_owl_buffer(const std::string& filename) {
    spdlog::info("ObjLoader: Loading OBJ file: {}", filename);
    obj::Result res = obj::ParseFile(filename);

    if (res.error) {
        spdlog::error("ObjLoader: Failed to load OBJ file: {}", res.error.code.message());
        return false;
    }

    if (!Triangulate(res)) {
        spdlog::error("ObjLoader: Failed to triangulate OBJ file: {}", filename);
        return false;
    }

    return true;
}

void ObjLoader::clear() {
    // Clear any loaded data
    data.vertices.clear();
    data.vert_indices.clear();
}