#include "Trace.cuh"
#include "trace/Ray.hpp"
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
int sample_env_discrete(const RayGenData& self, Trace::Record& prd) {
    const int size = self.env.size.x * self.env.size.y;
    const float float_idx = prd.random() * size;
    const int idx = static_cast<float>(float_idx);
    float remainder = float_idx - idx;

    if (remainder < self.env.alias_pdf[idx]) {
        prd.out.pdf = self.env.pdf[idx];
        return idx;
    } else {
        const int alias = self.env.alias_i[idx];
        prd.out.pdf = self.env.pdf[alias];
        return alias;
    }
}

inline __device__
vec3f sample_env_ray(const RayGenData& self, Trace::Record& prd) {
    int idx = sample_env_discrete(self, prd);

    int y = idx / self.env.size.x;
    int x = idx % self.env.size.x;

    // Convert to 2d texture coordinates
    const float u = (x + 0.5) / self.env.size.x;
    const float v = (y + 0.5) / self.env.size.y;

    // Convert to spherical
    const float theta = 2.0f * M_PIf * (u - 0.5f);
    const float phi = M_PIf * v;

    // Convert to cartesian
    const float ray_x = cosf(theta) * sinf(phi);
    const float ray_y = cosf(phi);
    const float ray_z = sinf(theta) * sinf(phi);

    return vec3f(ray_x, ray_y, ray_z);
}

inline __device__
float get_env_pdf(const RayGenData& self, const vec3f& dir, Trace::Record& prd) {
    // Convert to spherical coords
    const float theta = atan2f(dir.z, dir.x);
    const float phi = acosf(dir.y);

    // Convert to texture coordinates
    const float u = theta / (2.0f * M_PIf) + 0.5f;
    const float v = phi / M_PIf;

    // printf("u: %f, v: %f\n", u, v);

    // Convert to index in pdf table
    const int x = static_cast<int>(u * self.env.size.x);
    const int y = static_cast<int>(v * self.env.size.y);
    const int idx = y * self.env.size.x + x;

    // Get pdf
    return self.env.pdf[idx];
}

inline __device__
vec3f trace_path(const RayGenData& self, Ray& ray, Trace::Record& prd) {
    vec3f accum_attenuation = 1.f;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        traceRay(self.world, ray, prd);

        if (prd.out.scatter_event == Trace::ScatterEvent::RayMissed) {
            // Missed the scene, return background color
            return accum_attenuation * prd.out.attenuation;
        }
        else if (prd.out.scatter_event == Trace::ScatterEvent::RayCancelled) {
            // Hit light source
            return vec3f(0.f);
        }
        else {
            const vec3f brdf = prd.out.attenuation;
            const vec3f dir = prd.out.scattered_direction;
            const float pdf = (dot(normalize(prd.out.normal), normalize(dir)) / M_PIf);

            // TODO: I want this to be the pdf of the env map
            // const vec3f env_dir = sample_env_ray(self,prd);
            // const float env_pdf = get_env_pdf(self, dir, prd);
            // printf("pdf: %f %f\n", prd.out.pdf, env_pdf);

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
                dir,
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
    Trace::Record prd;
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
        prd.out.pdf = 1.f;
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

    Trace::Record& prd = owl::getPRD<Trace::Record>();
    prd.out.scatter_event = Trace::ScatterEvent::RayMissed;
    prd.out.attenuation = vec3f(bg_color.x, bg_color.y, bg_color.z);
}
