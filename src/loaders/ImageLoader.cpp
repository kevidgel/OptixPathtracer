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
}

ImageLoader::~ImageLoader() {
    if (config.cache && image_data) {
        stbi_image_free(const_cast<float*>(image_data));
    }
}

const float* ImageLoader::get_image_data(const std::string& file_path, int& width, int& height) {
    if (config.cache && image_data && file_path == this->file_path) {
        width = image_width;
        height = image_height;
        return image_data;
    }
    bool is_hdr = stbi_is_hdr(file_path.c_str()); // Check if the file is HDR
    const std::string type = is_hdr ? "HDR" : "LDR";

    spdlog::info("ImageLoader: Loading {} texture: {}", type, file_path);
    int channels;
    float* data = stbi_loadf(file_path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        spdlog::error("ImageLoader: Failed to load {} texture: {}", type, file_path);
        return nullptr; // Return an invalid texture
    }

    if (config.cache) {
        image_data = data;
        image_width = width;
        image_height = height;
    }
    this->file_path = file_path;
    return data;
}


OWLTexture ImageLoader::load_image_owl(const std::string& file_path, OWLContext ctx) {
    int width, height;
    const float* data = get_image_data(file_path, width, height);
    if (!data) return 0;

    const owl::vec4f* tex_data = reinterpret_cast<const owl::vec4f*>(data);

    // Load data
    OWLTexture texture =
        owlTexture2DCreate(
            ctx,
            OWL_TEXEL_FORMAT_RGBA32F,
            width, height,
            tex_data,
            OWL_TEXTURE_NEAREST,
            OWL_TEXTURE_CLAMP
        );

    if (!config.cache) {
        stbi_image_free(const_cast<float*>(image_data));
    }
    return texture;
}

// Luminance calculation for sRGB
inline
float luminance(const owl::vec4f& color) {
    return 0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z;
}

ImageLoader::AliasResult ImageLoader::build_alias(const std::string& file_path, OWLContext ctx) {
    int width, height;
    const float* data = get_image_data(file_path, width, height);
    if (!data) return std::nullopt;

    const owl::vec4f* tex_data = reinterpret_cast<const owl::vec4f*>(data);

    int size = width * height;
    spdlog::info("ImageLoader: Building alias table for texture: {} of size {}", file_path, size);
    auto pdf = std::make_unique<float[]>(size);
    float sum_weight = 0.0f;

    // Build pdf
    for (int y = 0; y < height; y++) {
        float theta = ((y + 0.5f) / height) * M_PI;
        float sin_theta = sin(theta);

        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            float weight = sin_theta * luminance(tex_data[i]);
            pdf[i] = weight;
            sum_weight += weight;
        }
    }

    // Normalize pdf * n
    for (int i = 0; i < size; i++) {
        pdf[i] *= (size / sum_weight);
    }

    // Alias index table
    auto alias_index = std::make_unique<int[]>(size);
    auto alias_pdf = std::make_unique<float[]>(size);
    std::vector<int> small, big;
    for (int i = 0; i < size; i++) {
        if (pdf[i] < 1.0f) {
            small.push_back(i);
        }
        else {
            big.push_back(i);
        }
    }

    while (!small.empty() && !big.empty()) {
        int s = small.back();
        int b = big.back();
        small.pop_back();
        big.pop_back();

        alias_index[s] = b;
        alias_pdf[s] = pdf[s];

        pdf[b] = (pdf[b] + pdf[s]) - 1.0f;
        if (pdf[b] < 1.0f) {
            small.push_back(b);
        }
        else {
            big.push_back(b);
        }
    }

    while (!small.empty()) {
        int s = small.back();
        small.pop_back();
        alias_pdf[s] = 1.0f;
        alias_index[s] = s;
    }

    while (!big.empty()) {
        int b = big.back();
        big.pop_back();
        alias_pdf[b] = 1.0f;
        alias_index[b] = b;
    }

    // Rebuild pdf
    for (int y = 0; y < height; y++) {
        float theta = ((y + 0.5f) / height) * M_PI;
        float sin_theta = sin(theta);

        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            float weight = sin_theta * luminance(tex_data[i]);
            pdf[i] = weight / sum_weight;
        }
    }

    float checksum = 0;
    for (int i = 0; i < size; i++) {
        checksum += pdf[i];
    }
    spdlog::info("ImageLoader: Alias table checksum: {}", checksum);

    if (!config.cache) {
        stbi_image_free(const_cast<float*>(image_data));
    }
    return Alias {
        std::move(pdf),
        std::move(alias_pdf),
        std::move(alias_index),
        {static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)}
    };
}