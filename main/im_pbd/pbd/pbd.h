/*
    2024-07-17 / imdongye

    From: https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
    Ref : JoltPhysics
*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <limbrary/glm_tools.h>
#include <limbrary/model_view/mesh.h>
#include <vector>


namespace pbd
{
    struct SoftBody;
    struct PhyScene;


    struct ICollider
    {
        float friction = 0.8f;
        float restitution = 0.8f;
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




    struct SoftBody: public lim::Mesh, public ICollider 
    {
        enum BendType {
            BT_NONE,
            BT_DISTANCE,
            BT_DIHEDRAL,
            BT_ISOMETRIC,
        };
        std::vector<glm::vec3> prev_x_s;
        std::vector<glm::vec3> x_s; // current poss (X)
        std::vector<glm::vec3> p_s; // new, predicted poss (P)
        std::vector<glm::vec3> v_s;
        std::vector<float>     w_s; // inv mass
        std::vector<glm::vec3> debug_dirs;
        std::vector<glm::uvec3>     ptcl_tris;
        // mached mesh poss idx
        // maximum 8 vertexs
        std::vector<glm::ivec4> idx_verts; 
        std::vector<glm::ivec4> idx_verts2;
        std::vector<glm::ivec4> idx_verts3;

        float inv_body_mass, inv_ptcl_mass;
        int nr_ptcls, nr_ptcl_tris;

        float pressure = 0.f;

        int nr_steps = 20;
        float ptcl_radius = 0.02f;

        std::vector<ConstraintDistance>      c_stretchs;
        std::vector<ConstraintDistance>      c_shears;
        std::vector<ConstraintDistance>      c_dist_bends;
        std::vector<ConstraintDihedralBend>  c_dih_bends;
        std::vector<ConstraintIsometricBend> c_iso_bends;

        struct Compliance {
            float dist, stretch_pct, shear_pct, bend_pct;
            float dih_bend, iso_bend;
            Compliance();
        };
        Compliance compliance;
        bool upload_to_buf = false;

        SoftBody();
        void addPtcl(const glm::vec3& p, float w, const glm::vec3& v);

        // nrShear => [0,2]
        SoftBody(const lim::Mesh& src, int nrShear = 1, BendType bendType = BT_NONE, float bodyMass = 1.f );
        void update(float dt, const PhyScene& scene);
        float getSdNor(const glm::vec3& p, glm::vec3& outNor) const;
    private:
        void uploadToBuf();
        void subStepConstraintProject(float dt);
        float getVolume() const;
        float getVolumeTimesSix() const;
        void applyCollision(float dt, const std::vector<ICollider*>& colliders);
        void applyCollisionInterSubstep(const std::vector<ICollider*>& colliders);
        void applyPressureImpulse(float dt);
    };


    struct PhyScene 
    {
        glm::vec3 G = {0.f, -9.8f, 0.f};
        float air_drag = 0.0001f;
        // this is refs
        std::vector<ICollider*> colliders;
        std::vector<SoftBody*> bodies;
        void update( float dt );
    };


    // struct SoftBodyGpu : public SoftBody
    // {
    //     bool upload_to_buf = false;
    //     GLuint buf_xw_s=0, buf_pw_s=0, buf_v_s=0, buf_f_s=0; // vec4
    //     GLuint buf_c_stretchs=0, buf_c_shears=0, buf_c_dist_bends=0;
    //     GLuint buf_c_dih_bends=0, buf_c_iso_bends=0, buf_c_g_volumes=0;
    //     GLuint vao_soft_body=0;


    //     SoftBodyGpu(const lim::Mesh& src, int nrShear = 1, BendType bendType = BT_NONE, float totalMass = 1.f);
    //     ~SoftBodyGpu();
    // };

    // struct SimulatorGpu
    // {
    //     int nr_steps = 20;
    //     float ptcl_radius = 0.02f;
    //     float air_drag = 0.0001f;
    //     SoftBody* body = nullptr;

    //     void update( float dt );
    // };
}

#endif