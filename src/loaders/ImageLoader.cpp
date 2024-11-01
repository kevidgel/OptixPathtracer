//
// Created by kevidgel on 10/30/24.
//

#include "ImageLoader.hpp"

#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "owl/common/math/vec.h"

ImageLoader::ImageLoader(const Config& config)
    : config(config) {
    if (!config.use_float) {
        spdlog::warn("Integer textures not supported yet");
    }
}

ImageLoader::~ImageLoader() {}

OWLTexture ImageLoader::load_image_owl(const std::string &file_path,
                                       OWLContext ctx) {
    bool is_hdr = stbi_is_hdr(file_path.c_str()); // Check if the file is HDR
    const std::string type = is_hdr ? "HDR" : "LDR";

    spdlog::info("ImageLoader: Loading {} texture: {}", type, file_path);
    int width, height, channels;
    float* data = stbi_loadf(file_path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        spdlog::error("ImageLoader: Failed to load {} texture: {}", type, file_path);
        return 0; // Return an invalid texture
    }

    owl::vec4f* tex_data = reinterpret_cast<owl::vec4f*>(data);

    // Load data
    OWLTexture texture =
        owlTexture2DCreate(
            ctx,
            OWL_TEXEL_FORMAT_RGBA32F,
            width, height,
            tex_data,
            OWL_TEXTURE_LINEAR,
            OWL_TEXTURE_CLAMP
        );

    stbi_image_free(data);
    return texture;
}