/**
* @file UberMaterial.hpp
*
* @brief Opting to use just one material for simplicity.
*/

#pragma once

#ifndef UBERMATERIAL_HPP
#define UBERMATERIAL_HPP

#include <owl/owl.h>
#include <owl/common/math/AffineSpace.h>
#include <owl/common/math/random.h>

#include "Ray.hpp"

using namespace owl;

namespace Material {
#ifdef __CUDA_ARCH__
    typedef LCG<8> Rng;

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
    vec3f random_in_cosine_weighted_hemisphere(Rng &rng) {
        const float r1 = rng();
        const float r2 = rng();
        float phi = 2.0f * M_PI * r1;
        const float y = sqrtf(1.0f - r2);
        const float x = cosf(phi) * sqrtf(r2);
        const float z = sinf(phi) * sqrtf(r2);
        return normalize(vec3f(x, y, z));
    }
#endif
    struct UberMaterial {
        vec3f albedo;
#ifdef __CUDA_ARCH__
        inline __device__
        static bool scatter(const UberMaterial &material,
                            const vec3f &P,
                            vec3f N,
                            RayData::Record &prd
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
            prd.out.attenuation = (material.albedo / (M_PIf)) * w_i.y;
            prd.out.pdf = w_i.y / M_PIf;
            return true;
        }
#endif
    };
}

#endif //UBERMATERIAL_HPP