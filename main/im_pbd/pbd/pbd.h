/*
    2024-07-17 / imdongye

*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <glm/glm.hpp>
#include <limbrary/model_view/mesh.h>
#include <vector>


namespace pbd
{
    struct SoftBody;

    struct ConstraintDistance {
        glm::uvec2 idx_ps;
        float ori_dist;
        float lambda = 0.f;
        ConstraintDistance(glm::uvec2 idxPs, float distance);
        void project(SoftBody& body, float alpha);
    };
    struct ConstraintBending {
        glm::uvec2 idx_ts;
        float ori_cangle; // cos(angle)
        float lambda = 0.f;
        ConstraintBending(glm::uvec2 idxTs, float cangle);
        void project(SoftBody& body, float alpha);
    };
    struct ConstraintVolume {
        float ori_volume;
        float lambda = 0.f;
        ConstraintVolume(float volume);
        void project(SoftBody& body, float alpha);
    };


    

    struct SoftBody {
        struct Settings {
            enum class BendType {
                None,
                Distance,
                CosAngle,
            };
            BendType bendType = BendType::CosAngle;
            // alpha : inv stiffness in XPBD
            float a_distance = 0.000001f;
            float a_bending  = 0.000001f;
            float a_volume   = 0.000001f;
        };
        Settings settings;

        // vec4(pos, invM) invM = w
        std::vector<glm::vec4> xw_s; // pos
        std::vector<glm::vec4> pw_s; // temp pos
        std::vector<glm::vec4> v0_s;
        std::vector<glm::uvec3> tris;
        glm::uint nr_ptcls, nr_tris;
        std::vector<ConstraintDistance> c_distances;
        std::vector<ConstraintBending> c_bendings;
        std::vector<ConstraintVolume> c_volumes;

        SoftBody(const lim::Mesh& src, Settings s = {});
        inline glm::vec3 getX(int idx) { return xw_s[idx]; }
        inline glm::vec3 getP(int idx) { return pw_s[idx]; }
        inline float     getW(int idx) { return xw_s[idx].w; }
        void updateP(float dt);
        void updateX(float dt);
        float getVolume();
    };


    struct Simulator {
        std::vector<SoftBody> bodies;
        void update( float dt );
    };
}

#endif