//
// Created by kevidgel on 10/30/24.
//

#ifndef IMAGELOADER_HPP
#define IMAGELOADER_HPP

#include <memory>
#include <optional>
#include <string>

#include "owl/owl.h"

class ImageLoader {
public:
    struct Config {
        bool cache = true;
    } config;

    ImageLoader(const Config& config);
    ~ImageLoader();

    OWLTexture load_image_owl(const std::string &file_path, OWLContext ctx);

    struct Alias {
        std::unique_ptr<float[]> pdf;
        std::unique_ptr<float[]> alias_pdf;
        std::unique_ptr<int[]> alias_i;
        std::pair<uint32_t, uint32_t> size;
    };
    using AliasResult = std::optional<Alias>;
    AliasResult build_alias(const std::string &file_path, OWLContext ctx);
private:
    const float* get_image_data(const std::string& file_path, int &width, int &height);

    std::string file_path;
    const float* image_data = nullptr;
    int image_width = 0;
    int image_height = 0;
};

#endif //IMAGELOADER_HPP
