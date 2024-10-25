/**
* @file Lambertian.hpp
*
* @brief Host/device shared Lambertian material definitions.
*/

#ifndef LAMBERTIAN_HPP
#define LAMBERTIAN_HPP

#include "Ray.hpp"

#include <owl/owl.h>
#include <owl/common/math/AffineSpace.h>
#include <owl/common/math/random.h>

using namespace owl;

namespace Material {
    struct Lambertian {
        vec3f albedo;
    };
#ifdef __CUDA_ARCH__

    typedef LCG<4> Rng;

    inline __device__
    vec3f random_in_unit_sphere(Rng &rng) {
        vec3f p;
        do {
            p = 2.0f * vec3f(rng(), rng(), rng()) - vec3f(1.f, 1.f, 1.f);
        } while (dot(p, p) >= 1.0f);
        return p;
    }

    /**
     * @brief Y-up
     */
    inline __device__
    vec3f random_in_cosine_weighted_hemisphere(Rng &rng, const vec3f &N) {
        const float r1 = rng();
        const float r2 = rng();
        float phi = 2.0f * M_PI * r1;
        const float y = sqrtf(1.0f - r2);
        const float x = cosf(phi) * sqrtf(r2);
        const float z = sinf(phi) * sqrtf(r2);
        return normalize(vec3f(x, y, z) + N);
    }

    inline __device__
    bool scatter(const Lambertian& material,
                 const vec3f &P,
                 vec3f N,
                 Trace::PerRayData &prd
                 ) {
        const vec3f ray_org = optixGetWorldRayOrigin();
        const vec3f ray_dir = optixGetWorldRayDirection();

        // Flip
        if (dot(N, ray_dir) > 0.0f) {
            N = -N;
        }
        N = normalize(N);

        // Create onb
        const vec3f b0 = N.y > 0.9999f ? vec3f(1, 0, 0) : vec3f(0, 1, 0);
        const vec3f b1 = normalize(cross(b0, N));
        const vec3f b2 = normalize(cross(N, b1));

        // Transform to normal local
        const vec3f dir = N + random_in_unit_sphere(prd.random);

        prd.out.scattered_origin = P;
        prd.out.scattered_direction = dir;
        prd.out.attenuation = material.albedo;
        prd.out.pdf = 1.0f / (4.0f * (M_PI));
        return true;
    }
#endif
}

#endif //LAMBERTIAN_HPP
