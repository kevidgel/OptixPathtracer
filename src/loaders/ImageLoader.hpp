//
// Created by kevidgel on 10/30/24.
//

#ifndef IMAGELOADER_HPP
#define IMAGELOADER_HPP

#include <string>

#include "owl/owl.h"

class ImageLoader {
public:
    struct Config {
        bool use_float = true; // Use float textures
        bool generate_mipmaps = false; // Generate mipmaps
    } config;

    ImageLoader(const Config& config);
    ~ImageLoader();

    OWLTexture load_image_owl(const std::string &file_path, OWLContext ctx);
};

#endif //IMAGELOADER_HPP
