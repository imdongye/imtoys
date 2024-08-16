#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <limbrary/gl_tools.h>
#include <limbrary/log.h>

using namespace glm;
using namespace pbd;
using std::vector;



SoftBodyGpu::SoftBodyGpu(lim::Mesh&& src, int nrShear, BendType bendType, float totalMass
    , bool refCloseVerts, bool isMakeNorsWithPtcl
) : SoftBody(std::move(src), nrShear, bendType, totalMass, refCloseVerts)
  , is_make_nors_with_ptcl(isMakeNorsWithPtcl)
{
    nr_thread_groups_by_ptcls = glim::fastIntCeil(nr_ptcls, nr_threads);
    nr_thread_groups_by_verts = glim::fastIntCeil(nr_verts, nr_threads);
}
SoftBodyGpu::~SoftBodyGpu()
{
    deinitGL();
}

/*
    no ref verts 는 poss 삭제하고 x_s를 연결한다.

*/
void SoftBodyGpu::initGL(bool withClearMem)
{
    Mesh::initGL(false);

    if( vert_to_ptcl.empty() ) {
        poss.clear(); poss.shrink_to_fit();
        tris.clear(); tris.shrink_to_fit();
        buf_x_s = buf_poss;
        buf_poss = 0;
        buf_ptcl_tris = buf_tris;
    }
    else {
        glGenBuffers(1, &buf_x_s);
        glBindBuffer(GL_ARRAY_BUFFER, buf_x_s);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*x_s.size(), x_s.data(), GL_STATIC_COPY);

        glGenBuffers(1, &buf_ptcl_tris);
        glBindBuffer(GL_ARRAY_BUFFER, buf_ptcl_tris);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ivec3)*ptcl_tris.size(), ptcl_tris.data(), GL_STATIC_COPY);

        glGenBuffers(1, &buf_vert_to_ptcl);
        glBindBuffer(GL_ARRAY_BUFFER, buf_vert_to_ptcl);
        glBufferData(GL_ARRAY_BUFFER, sizeof(int)*vert_to_ptcl.size(), vert_to_ptcl.data(), GL_STATIC_COPY);
    }
    

    glGenBuffers(1, &buf_p_s);
    glBindBuffer(GL_ARRAY_BUFFER, buf_p_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*p_s.size(), p_s.data(), GL_STATIC_COPY);


    glGenBuffers(1, &buf_v_s);
    glBindBuffer(GL_ARRAY_BUFFER, buf_v_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*v_s.size(), v_s.data(), GL_STATIC_COPY);

    // glGenBuffers(1, &buf_f_s);
    // glBindBuffer(GL_ARRAY_BUFFER, buf_f_s);
    // glBufferData(GL_ARRAY_BUFFER, elem_size*f_s.size(), f_s.data(), GL_DYNAMIC_COPY);


    glGenBuffers(1, &buf_w_s);
    glBindBuffer(GL_ARRAY_BUFFER, buf_w_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*w_s.size(), w_s.data(), GL_STATIC_COPY);


    glGenBuffers(1, &buf_debugs);
    glBindBuffer(GL_ARRAY_BUFFER, buf_debugs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*nr_ptcls, nullptr, GL_DYNAMIC_COPY);



    // make adjacent triangles per vertex
    vector<ivec2> offsetsPerPtcl;
    offsetsPerPtcl.reserve(nr_ptcls);
    vector<int> adjacentTriIdxs;
    adjacentTriIdxs.reserve(nr_verts*4);
    for( int i=0; i<nr_ptcls; i++ ) {
        int sOffset = (int)adjacentTriIdxs.size();
        for( int j=0; j<nr_tris; j++ ) {
            if( ptcl_tris[j].x == i || ptcl_tris[j].y == i || ptcl_tris[j].z == i ) {
                adjacentTriIdxs.push_back(j);
            }
        }
        int eOffset = (int)adjacentTriIdxs.size();
        offsetsPerPtcl.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_adj_tri_idxs);
    glBindBuffer(GL_ARRAY_BUFFER, buf_adj_tri_idxs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(int)*adjacentTriIdxs.size(), adjacentTriIdxs.data(), GL_STATIC_DRAW );

    glGenBuffers(1, &buf_adj_tri_idx_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_adj_tri_idx_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*offsetsPerPtcl.size(), offsetsPerPtcl.data(), GL_STATIC_DRAW );


    // constraint part ============================================

    // make distance constraints
    struct GpuConstraintDist {
        int idx_p;
        float ori_dist;
    };
    vector<GpuConstraintDist> gpuDistConstraints;
    gpuDistConstraints.reserve(c_stretchs.size()*2);


    // stretch distance
    gpuDistConstraints.clear();
    offsetsPerPtcl.clear();
    for( int idxP=0; idxP<nr_ptcls; idxP++ ) {
        int sOffset = gpuDistConstraints.size();
        for( const auto& c: c_stretchs ) { // stretch
            int idxOpp;
            if( c.idx_ps[0] == idxP ) {
                idxOpp = c.idx_ps[1];
            }
            else if( c.idx_ps[1] == idxP ) {
                idxOpp = c.idx_ps[0];
            }
            else {
                continue;
            }
            gpuDistConstraints.push_back({idxOpp, c.ori_dist});
        }
        int eOffset = gpuDistConstraints.size();
        offsetsPerPtcl.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_stretchs);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_stretchs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_stretch_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_stretch_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*offsetsPerPtcl.size(), offsetsPerPtcl.data(), GL_STATIC_DRAW );



    // shear distance
    gpuDistConstraints.clear();
    offsetsPerPtcl.clear();
    for( int idxP=0; idxP<nr_ptcls; idxP++ ) {
        int sOffset = gpuDistConstraints.size();
        for( const auto& c: c_shears ) { // shear
            int idxOpp;
            if( c.idx_ps[0] == idxP ) {
                idxOpp = c.idx_ps[1];
            }
            else if( c.idx_ps[1] == idxP ) {
                idxOpp = c.idx_ps[0];
            }
            else {
                continue;
            }
            gpuDistConstraints.push_back({idxOpp, c.ori_dist});
        }
        int eOffset = gpuDistConstraints.size();
        offsetsPerPtcl.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_shears);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_shears);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_shear_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_shear_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*offsetsPerPtcl.size(), offsetsPerPtcl.data(), GL_STATIC_DRAW );




    // bend distance
    gpuDistConstraints.clear();
    offsetsPerPtcl.clear();
    for( int idxP=0; idxP<nr_ptcls; idxP++ ) {
        int sOffset = gpuDistConstraints.size();
        for( const auto& c: c_dist_bends ) { // bend
            int idxOpp;
            if( c.idx_ps[0] == idxP ) {
                idxOpp = c.idx_ps[1];
            }
            else if( c.idx_ps[1] == idxP ) {
                idxOpp = c.idx_ps[0];
            }
            else {
                continue;
            }
            gpuDistConstraints.push_back({idxOpp, c.ori_dist});
        }
        int eOffset = gpuDistConstraints.size();
        offsetsPerPtcl.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_dist_bends);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_dist_bends);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_dist_bend_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_dist_bend_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*offsetsPerPtcl.size(), offsetsPerPtcl.data(), GL_STATIC_DRAW );


    // dihedral bending constraint
    if( c_dih_bends.empty()==false ) {
        vector<int> dihBendIdxs;
        dihBendIdxs.reserve(nr_ptcls*4);
        offsetsPerPtcl.clear();

        for( int idxP=0; idxP<nr_ptcls; idxP++ ) {
            int sOffset = dihBendIdxs.size();
            for( int i=0; i<c_dih_bends.size(); i++ ) {
                const auto& c = c_dih_bends[i];
                bool isIn = false;
                for( int j=0; j<4; j++ ) {
                    if( c.idx_ps[j] == idxP ) {
                        isIn = true;
                        break;
                    }
                }
                if( isIn ) {
                    dihBendIdxs.push_back(i);
                }
            }
            int eOffset = dihBendIdxs.size();
            offsetsPerPtcl.push_back(ivec2{sOffset, eOffset});
        }
        glGenBuffers(1, &buf_c_dih_bend_idx_offsets);
        glBindBuffer(GL_ARRAY_BUFFER, buf_c_dih_bend_idx_offsets);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*offsetsPerPtcl.size(), offsetsPerPtcl.data(), GL_STATIC_DRAW );
        glGenBuffers(1, &buf_c_dih_bend_idxs);
        glBindBuffer(GL_ARRAY_BUFFER, buf_c_dih_bend_idxs);
        glBufferData(GL_ARRAY_BUFFER, sizeof(int)*dihBendIdxs.size(), dihBendIdxs.data(), GL_STATIC_DRAW );
        glGenBuffers(1, &buf_c_dih_bends);
        glBindBuffer(GL_ARRAY_BUFFER, buf_c_dih_bends);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ConstraintDihedralBend)*c_dih_bends.size(), c_dih_bends.data(), GL_STATIC_DRAW );
    }
    // Todo: iso bend ...

}

void SoftBodyGpu::downloadXs()
{
    glBindBuffer(GL_ARRAY_BUFFER, buf_x_s);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*x_s.size(), x_s.data());
}


void SoftBodyGpu::deinitGL()
{
    lim::gl::safeDelBufs(&buf_x_s); // 0
    lim::gl::safeDelBufs(&buf_p_s); // 1
    lim::gl::safeDelBufs(&buf_v_s); // 2
    lim::gl::safeDelBufs(&buf_w_s); // 3
    lim::gl::safeDelBufs(&buf_debugs); // 3

    lim::gl::safeDelBufs(&buf_ptcl_tris);
    lim::gl::safeDelBufs(&buf_adj_tri_idxs);
    lim::gl::safeDelBufs(&buf_adj_tri_idx_offsets);

    lim::gl::safeDelBufs(&buf_vert_to_ptcl);

    lim::gl::safeDelBufs(&buf_c_stretch_offsets);
    lim::gl::safeDelBufs(&buf_c_stretchs);
    lim::gl::safeDelBufs(&buf_c_shear_offsets);
    lim::gl::safeDelBufs(&buf_c_shears);
    lim::gl::safeDelBufs(&buf_c_dist_bend_offsets);
    lim::gl::safeDelBufs(&buf_c_dist_bends); 

    lim::gl::safeDelBufs(&buf_c_points);

    lim::gl::safeDelBufs(&buf_c_dih_bend_idx_offsets);
    lim::gl::safeDelBufs(&buf_c_dih_bend_idxs);
    lim::gl::safeDelBufs(&buf_c_dih_bends); 

    Mesh::deinitGL();
}