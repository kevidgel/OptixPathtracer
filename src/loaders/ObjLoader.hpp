//
// Created by kevidgel on 10/27/24.
//

#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <span>
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

    enum class Attribute {
        Vertices,
        Normals,
        TexCoords,
        Indices,
        NormalIndices,
        TexCoordIndices,
    };

    struct Config {
        std::vector<uint32_t> meshes = {};
        LoadFlags loadFlags;
    };

    ObjLoader(const Config& config);
    ~ObjLoader();

    bool load(const std::string& filename);
    template<class T>
    std::span<const T> get(Attribute attribute) const {
        switch(attribute) {
            case Attribute::Vertices:
                if constexpr(std::is_same_v<T, vec3f>) {
                    return std::span<const T>(data.vertices);
                }
                break;
            case Attribute::Normals:
                if constexpr(std::is_same_v<T, vec3f>) {
                    return std::span<const T>(data.normals);
                }
                break;
            case Attribute::TexCoords:
                if constexpr(std::is_same_v<T, vec2f>) {
                    return std::span<const T>(data.texcoords);
                }
                break;
            case Attribute::Indices:
                if constexpr(std::is_same_v<T, vec3ui>) {
                    return std::span<const T>(data.indices);
                }
                break;
            case Attribute::NormalIndices:
                if constexpr(std::is_same_v<T, vec3ui>) {
                    return std::span<const T>(data.normal_indices);
                }
                break;
            case Attribute::TexCoordIndices:
                if constexpr(std::is_same_v<T, vec3ui>) {
                    return std::span<const T>(data.texcoord_indices);
                }
                break;
        }
        throw std::runtime_error("Invalid attribute type");
    }

    void clear();
private:
    Config config;

    struct Data {
        std::vector<vec3f> vertices;
        std::vector<vec3ui> indices;
        std::vector<vec3f> normals;
        std::vector<vec3ui> normal_indices;
        std::vector<vec2f> texcoords;
        std::vector<vec3ui> texcoord_indices;
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
