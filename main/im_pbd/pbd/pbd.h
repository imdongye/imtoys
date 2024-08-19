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

Todo:
    elastic bungee constraint : 당기는 힘만

*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <limbrary/glm_tools.h>
#include <limbrary/model_view/mesh.h>
#include <limbrary/program.h>
#include <vector>


namespace pbd
{
    struct SoftBody;
    struct PhyScene;
    struct PhySceneGpu;


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
        alignas(16)int idx_p;

        ConstraintPoint(const SoftBody& body, int idxP, const glm::vec3& _point);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintDistance 
    {
        glm::ivec2 idx_ps;
        float ori_dist;

        glm::vec3 dPi[2];
        ConstraintDistance(const SoftBody& body, const glm::ivec2& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintDihedralBend 
    {
        glm::ivec4 idx_ps; // edge, opp1, opp2
        alignas(16)float ori_angle;
        ConstraintDihedralBend(const SoftBody& body, const glm::ivec4& idxPs);
        void project(SoftBody& body, float alpha);
    };

    struct ConstraintIsometricBend 
    {
        glm::ivec4 idx_ps; // edge, opp1, opp2
        glm::mat4 Q;

        glm::vec3 dPi[4];
        ConstraintIsometricBend(const SoftBody& body, const glm::ivec4& idxPs);
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
        std::vector<glm::ivec3> ptcl_tris;
        // nr verts
        std::vector<int> vert_to_ptcl;

        // per ptcl
        std::vector<glm::vec3> prev_x_s;
        std::vector<glm::vec3> x_s; // current poss (X)
        std::vector<glm::vec3> p_s; // new, predicted poss (P)
        std::vector<glm::vec3> v_s;
        std::vector<float>     w_s; // inv ptcl mass or inv infinity mass (0)
        std::vector<glm::vec3> debug_dirs;


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

        void initGL(bool withClearMem = false);
        
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


/*
    if no ref verts
        buf_x_s = buf_poss
        buf_poss = 0,
        but buf_ptcl_tris = buf_tris, because buf_tris is used at draw

    SoftBodyGpu does not have make nors with verts (type 3)

    c_points not ensure multiple c_points on one particle
*/
    struct SoftBodyGpu : public SoftBody
    {
        // vec3
        GLuint buf_x_s=0; // 0
        GLuint buf_p_s=0; // 1
        GLuint buf_v_s=0; // 2
        GLuint buf_w_s=0; // 3
        GLuint buf_debugs=0; // 4

        GLuint buf_adj_tri_idx_offsets = 0;
        GLuint buf_adj_tri_idxs = 0;
        GLuint buf_ptcl_tris = 0;

        GLuint buf_vert_to_ptcl = 0;


        // distance constraint : struct{int targetIdx, float length} per ptcls
        // and offsets : ivec2
        GLuint buf_c_stretch_offsets=0,   buf_c_stretchs=0;
        GLuint buf_c_shear_offsets=0,     buf_c_shears=0;
        GLuint buf_c_dist_bend_offsets=0, buf_c_dist_bends=0;

        // point constraint : struct{vec3 point, float length, int idx_p} per constraints
        GLuint buf_c_points=0;

        GLuint buf_c_dih_bend_idx_offsets=0, buf_c_dih_bend_idxs=0, buf_c_dih_bends=0;

        inline static constexpr int nr_threads = 16;
        int nr_thread_groups_by_ptcls;
        int nr_thread_groups_by_verts;

        const bool is_make_nors_with_ptcl;


        SoftBodyGpu(lim::Mesh&& src, int nrShear = 1, BendType bendType = BT_NONE
            , float bodyMass = 1.f, bool refCloseVerts = false
            , bool isMakeNorsWithPtcl = true );
        ~SoftBodyGpu();

        void initGL(bool withClearMem = false);
        void deinitGL();
        void update(float dt, const PhySceneGpu& scene );

        void downloadXs();
    };

    struct PhySceneGpu
    {
        glm::vec3 G = {0.f, -9.8f, 0.f};
        float air_drag = 0.2f;

        lim::Program prog_pbd;
        lim::Program prog_0_update_p_s;
        lim::Program prog_1_project_dih_bend;
        lim::Program prog_1_project_dist;
        lim::Program prog_1_project_point;
        lim::Program prog_2_update_x_s;
        lim::Program prog_3_apply_collision;
        lim::Program prog_3_apply_pressure_impulse;
        lim::Program prog_4_make_ptcl_nors;
        lim::Program prog_4_update_vert_poss_nors;

        std::vector<ICollider*> colliders;
        std::vector<SoftBodyGpu*> bodies;

        PhySceneGpu();
        void update( float dt );
    };
}

#endif