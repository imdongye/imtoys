/*
    2024-07-17 / imdongye

*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <limbrary/glm_tools.h>
#include <limbrary/model_view/mesh.h>
#include <vector>


namespace pbd
{
    struct SoftBody;

    struct ConstraintDistance {
        glm::uvec2 idx_ps;
        float ori_dist, lambda;
        ConstraintDistance(glm::uvec2 idxPs, float distance);
        void project(SoftBody& body, float alpha);
    };
    struct ConstraintBending {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        float ori_angle;
        ConstraintBending(glm::uvec4 idxPs, float angle);
        void project(SoftBody& body, float alpha);
    };
    struct ConstraintIsometricBending {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        glm::mat4 Q;
        ConstraintIsometricBending(glm::uvec4& idxPs, glm::mat4& _Q);
        void project(SoftBody& body, float alpha);
    };
    struct ConstraintVolume {
        float ori_volume;
        ConstraintVolume(float volume);
        void project(SoftBody& body, float alpha);
    };


    

    struct SoftBody: public lim::Mesh {
        struct Settings {
            enum class BendType {
                None,
                Distance,
                CosAngle,
                Isometric,
            };
            bool update_buf = false;
            BendType bendType = BendType::CosAngle;
            // compliance to be alpha : inv stiffness in XPBD
            float a_distance = 0.001f;
            float a_bending  = 0.001f;
            float a_volume   = 0.001f;
        };
        
        Settings settings;

        // lim::Mesh poss => current poss (X)
        std::vector<glm::vec3> np_s; // new, predicted poss (P)
        std::vector<glm::vec3>  v_s;
        std::vector<float>      w_s; // inv mass
        glm::uint nr_ptcls, nr_tris;
        std::vector<ConstraintDistance> c_distances;
        std::vector<ConstraintBending>  c_bendings;
        std::vector<ConstraintIsometricBending>  c_i_bendings;
        std::vector<ConstraintVolume>   c_volumes;

        SoftBody(const lim::Mesh& src, Settings s = {});
        void updateP(float dt);
        void updateX(float dt);
        float getVolume();
    };


    struct Simulator {
        std::vector<SoftBody*> bodies;
        ~Simulator();
        void clear();
        void update( float dt );
    };
}

#endif