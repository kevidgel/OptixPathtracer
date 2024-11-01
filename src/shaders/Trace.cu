#include "Trace.cuh"
#include "Ray.hpp"
#include "geometry/Sphere.hpp"

#include <optix_device.h>

#define SAMPLES_PER_PIXEL 4
#define MAX_DEPTH 16

OPTIX_BOUNDS_PROGRAM(LambertianSpheres)(const void *geom_data, box3f &prim_bounds, const int prim_id) {
    Geometry::Sphere::bounds<Geometry::LambertianSpheresGeom>(geom_data, prim_bounds, prim_id);
}

OPTIX_INTERSECT_PROGRAM(LambertianSpheres)() {
    Geometry::Sphere::intersect<Geometry::LambertianSpheresGeom>();
}

OPTIX_CLOSEST_HIT_PROGRAM(LambertianSpheres)() {
    Geometry::Sphere::closest_hit<Geometry::LambertianSpheresGeom, Material::Lambertian>();
}

inline __device__
vec3f miss_color(const Ray& ray) {
    const vec3f ray_dir = normalize(ray.direction);
    const float t = 0.5f * (ray_dir.y + 1.0f);
    const vec3f c = (1.0f - t) * vec3f(1.0f) + t * vec3f(0.5f, 0.7f, 1.0f);
    return c;
}

inline __device__
vec3f trace_path(const RayGenData& self, Ray& ray, RayData::Record& prd) {
    vec3f accum_attenuation = 1.f;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        traceRay(self.world, ray, prd);

        if (prd.out.scatter_event == RayData::ScatterEvent::RayMissed) {
            // Missed the scene, return background color
            return accum_attenuation * prd.out.attenuation;
        }
        else if (prd.out.scatter_event == RayData::ScatterEvent::RayCancelled) {
            // Hit light source
            return vec3f(0.f);
        }
        else {
            const float pdf = prd.out.pdf;
            const vec3f brdf = prd.out.attenuation;

            const vec3f throughput = brdf / pdf;
            float roulette_weight =
                1.0f - clamp(max(max(throughput.x, throughput.y), throughput.z), 0.0f, 1.0f);
            if (depth <= 3) roulette_weight = 0.f;

            if (prd.random() < roulette_weight) {
                return vec3f(0.f);
            }

            accum_attenuation *= (throughput / (1.0f - roulette_weight));

            ray = Ray(
                prd.out.scattered_origin,
                prd.out.scattered_direction,
                1e-3f,
                1e10f
                );
        }
    }

    return vec3f(0.f);
}

OPTIX_RAYGEN_PROGRAM(RayGen)() {
    const RayGenData& self = owl::getProgramData<RayGenData>();
    // Get our pixel indices
    const vec2i pixel_id = owl::getLaunchIndex();
    const int pboOfs = pixel_id.x + self.pbo_size.x * pixel_id.y;

    // Build primary rays
    RayData::Record prd;
    prd.random.init(pboOfs, self.launch->frame.id);

    vec3f color = 0.f;
    for (int sample_id = 0; sample_id < SAMPLES_PER_PIXEL; sample_id++) {
        // Build primary ray
        Ray ray;

        const vec2f pixel_offset(prd.random(), prd.random());
        const vec2f uv = (vec2f(pixel_id) + pixel_offset) / vec2f(self.pbo_size);
        const vec3f origin = self.launch->camera.pos;
        const vec3f direction = self.launch->camera.dir_00
            + uv.x * self.launch->camera.dir_du
            + uv.y * self.launch->camera.dir_dv;

        ray.origin = origin;
        ray.direction = normalize(direction);

        // Trace
        color += trace_path(self, ray, prd);
    }

    if (self.launch->dirty) {
        self.pbo_ptr[pboOfs] = vec4f(color * (1.f / SAMPLES_PER_PIXEL), 1.f);
    } else {
        self.pbo_ptr[pboOfs] += vec4f(color * (1.f / SAMPLES_PER_PIXEL), 1.f);
    }
}

// OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)() {
//     vec3f& prd = owl::getPRD<vec3f>();
//
//     const TrianglesGeomData &self = owl::getProgramData<TrianglesGeomData>();
//
//     // Compute normal
//     const int prim_id = optixGetPrimitiveIndex();
//     const vec3i index = self.index[prim_id];
//     const vec3f v0 = self.vertex[index.x];
//     const vec3f v1 = self.vertex[index.y];
//     const vec3f v2 = self.vertex[index.z];
//     const vec3f n = normalize(cross(v1 - v0, v2 - v0));
//
//     const vec3f ray_dir = optixGetWorldRayDirection();
// }

OPTIX_MISS_PROGRAM(Miss)() {
    const MissProgData &self = owl::getProgramData<MissProgData>();
    vec3f dir = optixGetWorldRayDirection();
    dir = normalize(dir);

    // Convert to spherical coords
    const float theta = atan2f(dir.z, dir.x);
    const float phi = acosf(dir.y);

    // Convert to texture coordinates
    const float u = theta / (2.0f * M_PIf) + 0.5f;
    const float v = phi / M_PIf;

    // Retrieve environment map color
    const vec4f bg_color = tex2D<float4>(self.env_map, u, v);

    RayData::Record& prd = owl::getPRD<RayData::Record>();
    prd.out.scatter_event = RayData::ScatterEvent::RayMissed;
    prd.out.attenuation = vec3f(bg_color.x, bg_color.y, bg_color.z);
}
