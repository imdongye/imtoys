/*
    2024-07-17 / imdongye

    From: https://matthias-research.github.io/pages/publications/posBasedDyn.pdf

*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <limbrary/glm_tools.h>
#include <limbrary/model_view/mesh.h>
#include <vector>


namespace pbd
{
    struct SoftBody;

    struct ConstraintPoint
    {
        glm::vec3 target;
        int idx;
        ConstraintPoint(int _idx, const glm::vec3& _target);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintDistance 
    {
        glm::uvec2 idx_ps;
        float ori_dist;
        glm::vec3 dPi[2];
        ConstraintDistance(const SoftBody& body, const glm::uvec2& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintDihedralBend 
    {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        float ori_angle;
        glm::vec3 dPi[4];
        ConstraintDihedralBend(const SoftBody& body, const glm::uvec4& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintIsometricBend 
    {
        glm::uvec4 idx_ps; // edge, opp1, opp2
        glm::mat4 Q;
        glm::vec3 dPi[4];
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
        float friction = 1.f;
        float restitution = 1.f;
        virtual float getSdNor( const glm::vec3& p, glm::vec3& outNor ) const = 0;
    };
    struct ColliderPlane: public ICollider
    {
        glm::vec3 n;
        float r;
        ColliderPlane(const glm::vec3& _n = {0,1,0}, float _r = 0.f);
        virtual float getSdNor( const glm::vec3& p, glm::vec3& outNor ) const override;
    };
    struct ColliderSphere: public ICollider
    {
        glm::vec3 c;
        float r;
        ColliderSphere(const glm::vec3& _c, float _r = 0.5f);
        virtual float getSdNor( const glm::vec3& p, glm::vec3& outNor ) const override;
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
        std::vector<glm::vec3> tp_s; // temp for jocobi
        std::vector<glm::vec3> np_s; // new, predicted poss (P)
        std::vector<glm::vec3>  v_s;
        std::vector<glm::vec3>  f_s;
        std::vector<float>      w_s; // inv mass
        float total_mass, inv_ptcl_mass;
        int nr_ptcls, nr_tris;
        float friction = 0.95f;
        float restitution = 0.85f;

        std::vector<ConstraintDistance>      c_stretchs;
        std::vector<ConstraintDistance>      c_shears;
        std::vector<ConstraintDistance>      c_dist_bends;
        std::vector<ConstraintDihedralBend>  c_dih_bends;
        std::vector<ConstraintIsometricBend> c_iso_bends;
        std::vector<ConstraintGlobalVolume>  c_g_volumes;

        struct Compliance {
            float stretch, shear, dist_bend;
            float dih_bend, iso_bend;
            float glo_volume;
            Compliance();
        };
        Compliance compliance;
        bool upload_to_buf = false;

        SoftBody();
        void addPtcl(const glm::vec3& p, float w, const glm::vec3& v);

        // nrShear => [0,2]
        SoftBody(const lim::Mesh& src, int nrShear = 1, BendType bendType = BendType::Dihedral, float totalMass = 1.f);
        void update(float dt);
        float getVolume() const;
        void applyDeltaP(int idx, float lambda, const glm::vec3& dC);
    };

    struct SoftBodyGpu : public SoftBody
    {
        bool upload_to_buf = false;
        GLuint buf_xw_s=0, buf_pw_s=0, buf_v_s=0, buf_f_s=0; // vec4
        GLuint buf_c_stretchs=0, buf_c_shears=0, buf_c_dist_bends=0;
        GLuint buf_c_dih_bends=0, buf_c_iso_bends=0, buf_c_g_volumes=0;
        GLuint vao_soft_body=0;


        SoftBodyGpu(const lim::Mesh& src, int nrShear = 1, BendType bendType = BendType::Dihedral, float totalMass = 1.f);
        ~SoftBodyGpu();
    };


    struct Simulator 
    {
        int nr_steps = 20;
        float ptcl_radius = 0.02f;
        float air_drag = 0.0001f;
        std::vector<ICollider*> static_colliders;
        std::vector<SoftBody*> bodies;
        ~Simulator();
        void clearRefs();
        void update( float dt );
    };
    struct SimulatorGpu
    {
        int nr_steps = 20;
        float ptcl_radius = 0.02f;
        float air_drag = 0.0001f;
        SoftBody* body = nullptr;

        void update( float dt );
    };
}

#endif