#include "Trace.cuh"
#include "trace/Ray.hpp"
#include "geometry/Sphere.hpp"

#include <optix_device.h>

#include "geometry/TriangleMesh.hpp"

#define SAMPLES_PER_PIXEL 2
#define MAX_DEPTH 50

/**
 * @brief Y-up
 */
inline __device__
vec3f random_in_cosine_weighted_hemisphere(LCG<8> &rng) {
    const float r1 = rng();
    const float r2 = rng();
    float phi = 2.0f * M_PI * r1;
    const float y = sqrtf(1.0f - r2);
    const float x = cosf(phi) * sqrtf(r2);
    const float z = sinf(phi) * sqrtf(r2);
    return normalize(vec3f(x, y, z));
}

__device__
static bool scatter(const vec3f diffuse,
                    const vec3f &P,
                    vec3f N,
                    Trace::Record &prd
) {
    const vec3f W_o = optixGetWorldRayDirection();

    // Flip
    if (dot(N, W_o) > 0.0f) {
        N = -N;
    }
    N = normalize(N);

    // Create onb
    const vec3f ref = N.y > 0.9999f ? vec3f(1, 0, 0) : vec3f(0, 1, 0);
    const vec3f T = normalize(cross(ref, N));
    const vec3f B = normalize(cross(N, T));

    // T*x + N*y + B*z to convert local (x,y,z) to world
    const vec3f w_i = random_in_cosine_weighted_hemisphere(prd.random);
    const vec3f W_i = w_i.x * T + w_i.y * N + w_i.z * B;

    // Update the record
    // Note we include the cos(theta) term in the attenuation
    prd.out.scattered_origin = P;
    prd.out.scattered_direction = W_i;
    prd.out.attenuation = (diffuse / (M_PIf)) * w_i.y;
    prd.out.normal = N;
    prd.out.pdf = w_i.y / M_PIf;
    return true;
}

OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)() {
    const auto& self = owl::getProgramData<Geometry::TriangleMesh>();
    Trace::Record& prd = owl::getPRD<Trace::Record>();
    // Ray data
    const vec3f ray_org = optixGetWorldRayOrigin();
    const vec3f ray_dir = optixGetWorldRayDirection();
    const float hit_t = optixGetRayTmax();
    const vec3f hit_point = ray_org + hit_t * ray_dir;
    // Tri data
    const int prim_id = optixGetPrimitiveIndex();
    const vec3ui nindex = self.normal_indices[prim_id];
    const vec3f& n0 = self.normals[nindex.x];
    const vec3f& n1 = self.normals[nindex.y];
    const vec3f& n2 = self.normals[nindex.z];
    const vec2f bary = optixGetTriangleBarycentrics();
    const vec3f N = normalize(bary.x * n0 + bary.y * n1 + (1.0f - bary.x - bary.y) * n2);
    // Scatter
    // const vec3ui tindex = self.texcoord_indices[prim_id];
    // const vec2f& t0 = self.tex_coords[tindex.x];
    // const vec2f& t1 = self.tex_coords[tindex.y];
    // const vec2f& t2 = self.tex_coords[tindex.z];
    // vec2f uv = bary.x * t0 + bary.y * t1 + (1.0f - bary.x - bary.y) * t2;
    // uv.x = fmodf(uv.x, 1.0f);
    // uv.y = fmodf(uv.y, 1.0f);
    scatter({0.8, 0.8, 0.8}, hit_point, N, prd);
    prd.out.scatter_event = Trace::ScatterEvent::RayScattered;
}

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

// Currently just a cosine-weighted hemisphere
inline __device__
float bsdf_pdf(const vec3f& W_i, const vec3f& N) {
    return max(0.0f, dot(W_i, N) / M_PIf);
}

inline __device__
int sample_env_discrete(const RayGenData& self, Trace::Record& prd) {
    const int size = self.env.size.x * self.env.size.y;
    int idx = prd.random() * size;
    idx = min(idx, size - 1);

    if (prd.random() < self.env.alias_pdf[idx]) {
        return idx;
    } else {
        const int alias = self.env.alias_i[idx];
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

    // Convert to index in pdf table
    const int x = static_cast<int>(u * self.env.size.x);
    const int y = static_cast<int>(v * self.env.size.y);
    int idx = y * self.env.size.x + x;
    idx = min(idx, self.env.size.x * self.env.size.y - 1);

    // Get pdf
    return self.env.pdf[idx];
}

inline __device__
bool traceShadowRay(vec3f origin, vec3f direction, Trace::Record& prd) {
    const float tmin = 1e-3f;
    const float tmax = 1e10f;
    Ray ray(origin, direction, tmin, tmax);
    prd.out.scatter_event = Trace::ScatterEvent::RayMissed;
    traceRay(owl::getProgramData<RayGenData>().world, ray, prd);
    // True if missed
    return prd.out.scatter_event != Trace::ScatterEvent::RayScattered;
}

inline __device__
vec3f trace_path(const RayGenData& self, Ray& ray, Trace::Record& prd) {
    vec3f accum_attenuation = 1.f;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        traceRay(self.world, ray, prd);

        // BG
        if (prd.out.scatter_event == Trace::ScatterEvent::RayMissed) {
            // Missed the scene, return background color
            return accum_attenuation * prd.out.attenuation;
        }

        // Light (not implemented)
        if (prd.out.scatter_event == Trace::ScatterEvent::RayCancelled) {
            return vec3f(0.f);
        }

        const vec3f brdf = prd.out.attenuation;
        vec3f dir = prd.out.scattered_direction;
        const float pdf = bsdf_pdf(dir, prd.out.normal);

        const vec3f throughput = brdf / pdf;
        float roulette_weight =
            1.0f - clamp(max(max(throughput.x, throughput.y), throughput.z), 0.3f, 1.0f);
        if (depth <= 3) roulette_weight = 0.f;

        if (prd.random() < roulette_weight) {
            return vec3f(0.f);
        }

        vec3f l = (throughput / (1.0f - roulette_weight));
        accum_attenuation *= l;

        ray = Ray(
            prd.out.scattered_origin,
            dir,
            1e-3f,
            1e10f
        );
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

    if (isnan(color.x) || isnan(color.y) || isnan(color.z)) {
        color = vec3f(0.f);
    }

    if (isinf(color.x) || isinf(color.y) || isinf(color.z)) {
        color = vec3f(0.f);
    }

    if (self.launch->dirty) {
        self.pbo_ptr[pboOfs] = vec4f(color * (1.f / SAMPLES_PER_PIXEL), 1.f);
    } else {
        self.pbo_ptr[pboOfs] += vec4f(color * (1.f / SAMPLES_PER_PIXEL), 1.f);
    }
}

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
