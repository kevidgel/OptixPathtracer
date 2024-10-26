#include "Trace.cuh"
#include "Ray.hpp"
#include "geometry/Sphere.hpp"

#include <optix_device.h>

#define SAMPLES_PER_PIXEL 10
#define MAX_DEPTH 50

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
vec3f trace_path(const RayGenData& self, Ray& ray, Trace::PerRayData& prd) {
    vec3f accum_attenuation = 1.f;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        traceRay(self.world, ray, prd);

        if (prd.out.scatter_event == Trace::ScatterEvent::RayMissed) {
            return accum_attenuation * miss_color(ray);
        }
        else if (prd.out.scatter_event == Trace::ScatterEvent::RayCancelled) {
            return vec3f(0.f);
        }
        else {
            const float pdf = 1.0;
            const vec3f brdf = prd.out.attenuation;

            accum_attenuation *= brdf / pdf;
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
    Trace::PerRayData prd;
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
    const MissProgData& self = owl::getProgramData<MissProgData>();
    Trace::PerRayData& prd = owl::getPRD<Trace::PerRayData>();
    prd.out.scatter_event = Trace::ScatterEvent::RayMissed;
}
