//
// Created by kevidgel on 10/24/24.
//

#ifndef RAY_HPP
#define RAY_HPP

#include <owl/owl.h>
#include <owl/common/math/random.h>

#ifdef __CUDA_ARCH__
using namespace owl;

namespace Trace {
    typedef LCG<8> Random;

    typedef enum {
        RayScattered,
        RayCancelled,
        RayMissed,
    } ScatterEvent;

    struct PerRayData {
        Random random;
        struct {
            ScatterEvent scatter_event;
            vec3f scattered_origin;
            vec3f scattered_direction;
            vec3f attenuation;
            float pdf;
        } out;
    };
}
#endif

#endif //RAY_HPP
