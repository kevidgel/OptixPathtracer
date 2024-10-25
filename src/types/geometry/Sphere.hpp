/**
* @file Sphere.hpp
*
* @brief Host/device shared sphere definitions.
*/

#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "Ray.hpp"
#include "materials/Lambertian.hpp"
using namespace owl;

namespace Geometry {
    struct Sphere {
        vec3f center;
        float radius;

#ifdef __CUDA_ARCH__
        template<typename SpheresGeom>
        inline __device__
        static void bounds(const void* geom_data, box3f &prim_bounds, const int prim_id) {
            const SpheresGeom& self = *static_cast<const SpheresGeom*>(geom_data);
            const Sphere sphere = self.prims[prim_id].sphere;
            prim_bounds = box3f()
                .extend(sphere.center - vec3f(sphere.radius))
                .extend(sphere.center + vec3f(sphere.radius));
        }

        template<typename SpheresGeom>
        inline __device__
        static void intersect() {
            const int prim_id = optixGetPrimitiveIndex();
            const auto &self = owl::getProgramData<SpheresGeom>().prims[prim_id];

            const vec3f ray_org = optixGetObjectRayOrigin();
            const vec3f ray_dir = optixGetObjectRayDirection();
            float hit_t = optixGetRayTmax();
            const float tmin = optixGetRayTmin();

            const vec3f oc = ray_org - self.sphere.center;
            const float a = dot(ray_dir, ray_dir);
            const float b = dot(oc, ray_dir);
            const float c = dot(oc, oc) - self.sphere.radius * self.sphere.radius;
            const float discriminant = b * b - a * c;

            // No hit
            if (discriminant < 0.f) return;

            const float sol1 = (-b - sqrtf(discriminant)) / a;
            if (sol1 < hit_t && sol1 > tmin) {
                hit_t = sol1;
            }

            const float sol2 = (-b + sqrtf(discriminant)) / a;
            if (sol2 < hit_t && sol2 > tmin) {
                hit_t = sol2;
            }

            // Find the closest hit
            if (hit_t < optixGetRayTmax()) {
                optixReportIntersection(hit_t, 0);
            }
        }

        inline __device__
        static vec3f to_world(const vec3f& P) {
            return (vec3f) optixTransformPointFromObjectToWorldSpace(P);
        }

        template<typename SpheresGeom>
        inline __device__
        static void closest_hit() {
            const int prim_id = optixGetPrimitiveIndex();
            const auto &self = owl::getProgramData<SpheresGeom>().prims[prim_id];

            Trace::PerRayData &prd = owl::getPRD<Trace::PerRayData>();

            const vec3f ray_org = optixGetWorldRayOrigin();
            const vec3f ray_dir = optixGetWorldRayDirection();
            const float hit_t = optixGetRayTmax();
            const vec3f hit_point = ray_org + hit_t * ray_dir;
            const vec3f N = hit_point - to_world(self.sphere.center);

            prd.out.scatter_event = scatter(self.material, hit_point, N, prd) ? Trace::RayScattered : Trace::RayMissed;
        }
#endif
    };

    struct LambertianSphere {
        Sphere sphere;
        Material::Lambertian material;
    };

    struct LambertianSpheresGeom {
        LambertianSphere *prims;
    };
}

#endif //SPHERE_HPP