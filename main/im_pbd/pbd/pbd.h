/*
    2024-07-17 / imdongye

From:
    https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
Ref:
    JoltPhysics
Note:
    Ptcl in SoftBody can referrence Vertexs in Mesh
    if that
        uploadToBuf do updateVerts for poss and upload poss
    else
        clear poss, tris
        and uploadToBuf do upload x_s

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
        virtual float getSdNor( const glm::vec3& p, glm::vec3& outNor ) const final;
    };
    struct ColliderSphere: public ICollider
    {
        glm::vec3 c;
        float r;
        ColliderSphere(const glm::vec3& _c, float _r = 0.5f);
        virtual float getSdNor( const glm::vec3& p, glm::vec3& outNor ) const final;
    };




    struct ConstraintPoint
    {
        glm::vec3 point;
        float ori_dist;
        int idx_p;
        ConstraintPoint(const SoftBody& body, int idxP, const glm::vec3& _point);
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

    struct ConstraintVolume
    {
        float ori_six_volume; // need init from body
        float cur_six_volume;
        void applyImpulse(SoftBody& body, float pressure, float dt);
        void projectImpulse(SoftBody& body, float pressure, float dt);
        void projectXpbd(SoftBody& body, float pressure, float alpha);
    };




    struct SoftBody: public lim::Mesh, public ICollider 
    {
        struct ConstraintParams {
            float inv_stiff_dist, stretch_pct, shear_pct, bend_pct;
            float inv_stiff_dih_bend, inv_stiff_iso_bend, inv_stiff_point;
            float inv_stiff_volume, pressure;
            ConstraintParams();
        };
        ConstraintParams params;

        float inv_body_mass, inv_ptcl_mass;
        int nr_ptcls;

        int nr_steps = 20;
        float ptcl_radius = 0.02f;


        // nr tris
        std::vector<glm::uvec3> ptcl_tris;
        // nr verts
        std::vector<glm::uint> idx_verts;

        // per ptcl
        std::vector<glm::vec3> prev_x_s;
        std::vector<glm::vec3> x_s; // current poss (X)
        std::vector<glm::vec3> p_s; // new, predicted poss (P)
        std::vector<glm::vec3> v_s;
        std::vector<float>     w_s; // inv ptcl mass or inv infinity mass (0)
        std::vector<glm::vec3> debug_dirs;
        std::vector<glm::uvec2> idx_verts_part_infos; // offset(start), end for idx_verts


        std::vector<ConstraintPoint>         c_points;
        std::vector<ConstraintDistance>      c_stretchs;
        std::vector<ConstraintDistance>      c_shears;
        std::vector<ConstraintDistance>      c_dist_bends;
        std::vector<ConstraintDihedralBend>  c_dih_bends;
        std::vector<ConstraintIsometricBend> c_iso_bends;
        ConstraintVolume c_volume;

        enum BendType : int {
            BT_NONE,
            BT_DISTANCE,
            BT_DIHEDRAL,
            BT_ISOMETRIC,
        };
		SoftBody(const Mesh& src) = delete;
        // nrShear => [0,2]
        SoftBody(lim::Mesh&& src, int nrShear = 1, BendType bendType = BT_NONE
            , float bodyMass = 1.f, bool refCloseVerts = false );
        void update(float dt, const PhyScene& scene);
        virtual float getSdNor(const glm::vec3& p, glm::vec3& outNor) const final;

        void initGL();

        
        void updateNorsAndUpload();
        void updatePossAndNorsWithPtclAndUpload();
        void updatePossAndNorsWithVertAndUpload();

        SoftBody();
        void addPtcl(const glm::vec3& p, float w, const glm::vec3& v);

        float getVolume() const;
        float getVolumeTimesSix() const;
    private:
        void subStepConstraintProject(float dt);
        void applyCollision(float dt, const std::vector<ICollider*>& colliders);
        void applyCollisionInterSubstep(const std::vector<ICollider*>& colliders);
    };


    struct PhyScene 
    {
        glm::vec3 G = {0.f, -9.8f, 0.f};
        float air_drag = 0.2f;
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