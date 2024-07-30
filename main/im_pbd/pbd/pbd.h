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

    struct ConstraintFix
    {
        glm::vec3 p;
        int idx;
        ConstraintFix(int _idx, const glm::vec3& _p);
        void project(SoftBody& body);
    };

    struct ConstraintDistance 
    {
        glm::uvec2 idx_ps;
        float ori_dist;
        ConstraintDistance(const SoftBody& body, const glm::uvec2& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintDihedralBend 
    {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        float ori_angle;
        glm::vec3 dCi[4];
        ConstraintDihedralBend(const SoftBody& body, const glm::uvec4& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintIsometricBend 
    {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        glm::mat4 Q;
        glm::vec3 dCi[4];
        ConstraintIsometricBend(const SoftBody& body, const glm::uvec4& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintGlobalVolume 
    {
        float ori_volume;
        std::vector<glm::vec3> dCi;
        ConstraintGlobalVolume(const SoftBody& body);
        void project(SoftBody& body, float alpha);
    };

    struct ICollider
    {
        float friction;
        float restitution;
        glm::vec3 vel;
        virtual float getSD(const glm::vec3& target) const;
        virtual glm::vec4 getNorh(const glm::vec3& target) const;
    };
    struct ColliderPlane: public ICollider
    {
        glm::vec3 p;
        glm::vec3 n;
        ColliderPlane(const glm::vec3& _p, const glm::vec3& _n);
        virtual float getSD(const glm::vec3& target) const override;
        virtual glm::vec4 getNorh(const glm::vec3& target) const override;
    };
    struct ColliderSphere: public ICollider
    {
        glm::vec3 p;
        glm::vec3 n;
        ColliderPlane(const glm::vec3& _p, const glm::vec3& _n);
        float getSD(const glm::vec3& target) const;
    };

    struct Ptcl
    {
        glm::vec3 x, p, v, f;
        float w, min_col_dist;
        glm::vec4 cs;
        int idx_col;
    };

    struct SoftBody: public lim::Mesh 
    {
        enum class BendType {
            None,
            Distance,
            Dihedral,
            Isometric,
        };
        // lim::Mesh poss => current poss (X)
        std::vector<glm::vec3> np_s; // new, predicted poss (P)
        std::vector<glm::vec3>  v_s;
        std::vector<glm::vec3>  f_s;
        std::vector<float>      w_s; // inv mass
        int nr_ptcls, nr_tris;

        std::vector<ConstraintDistance>      c_stretchs;
        std::vector<ConstraintDistance>      c_shears;
        std::vector<ConstraintDistance>      c_dist_bends;
        std::vector<ConstraintDihedralBend>  c_dih_bends;
        std::vector<ConstraintIsometricBend> c_iso_bends;
        std::vector<ConstraintGlobalVolume>  c_g_volumes;
        std::vector<ConstraintFix>           c_fixes;

        struct Compliance {
            float stretch, shear, dist_bend;
            float dih_bend, iso_bend;
            float glo_volume;
            Compliance();
        };
        Compliance compliance;

        bool update_buf = false;

        // nrShear => [0,2]
        SoftBody(const lim::Mesh& src, int nrShear = 1, BendType bendType = BendType::Dihedral);
        void updateP(float dt);
        void updateX(float dt);
        float getVolume() const;
        void applyDeltaP(int idx, float lambda, const glm::vec3& dC);
    };


    struct Simulator 
    {
        int nr_steps = 20;
        float ptcl_radius = 0.02f;
        std::vector<ICollider*> static_colliders;
        std::vector<SoftBody*> bodies;
        ~Simulator();
        void clear();
        void updateCollision();
        void update( float dt );
    };
}

#endif