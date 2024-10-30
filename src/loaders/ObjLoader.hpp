//
// Created by kevidgel on 10/27/24.
//

#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <owl/common/math/vec.h>
#include <rapidobj/rapidobj.hpp>

using namespace owl;
namespace obj = rapidobj;

class ObjLoader {
public:
    enum class LoadFlags : std::uint8_t {
        Vertices = 1 << 0,
        Normals = 1 << 1,
        TexCoords = 1 << 2,
    };

    struct Config {
        std::vector<uint32_t> meshes = {};
        LoadFlags loadFlags;
    };

    ObjLoader(const Config& config);
    ~ObjLoader();

    bool load(const std::string& filename);
    bool load_to_owl_buffer(const std::string& filename);
    void clear();
private:
    Config config;

    struct Data {
        std::vector<vec3f> vertices;
        std::vector<vec3ui> vert_indices;
        std::vector<vec3f> normals;
        std::vector<vec2f> texcoords;
    } data;
};

inline ObjLoader::LoadFlags operator|(ObjLoader::LoadFlags lhs, ObjLoader::LoadFlags rhs) {
    return static_cast<ObjLoader::LoadFlags>(
        static_cast<std::underlying_type_t<ObjLoader::LoadFlags>>(lhs) |
        static_cast<std::underlying_type_t<ObjLoader::LoadFlags>>(rhs)
    );
}

inline ObjLoader::LoadFlags operator&(ObjLoader::LoadFlags lhs, ObjLoader::LoadFlags rhs) {
    return static_cast<ObjLoader::LoadFlags>(
        static_cast<std::underlying_type_t<ObjLoader::LoadFlags>>(lhs) &
        static_cast<std::underlying_type_t<ObjLoader::LoadFlags>>(rhs)
    );
}

#endif //OBJLOADER_HPP
