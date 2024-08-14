#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <limbrary/gl_tools.h>
#include <limbrary/log.h>

using namespace glm;
using namespace pbd;
using std::vector;



SoftBodyGpu::SoftBodyGpu(lim::Mesh&& src, int nrShear, BendType bendType, float totalMass, bool refCloseVerts)
    : SoftBody(std::move(src), nrShear, bendType, totalMass, refCloseVerts)
{
    nr_thread_groups = glim::fastIntCeil(nr_ptcls, nr_threads);
}
SoftBodyGpu::~SoftBodyGpu()
{
    deinitGL();
}

/*
    poss, nors는 ssbo로 접근해야하기때문에 vec4로 만들고 vec3 stride로 vao에 연결한다

    no ref verts 는 poss 삭제하고 x_s를 연결한다.

*/
void SoftBodyGpu::initGL(bool withClearMem)
{
    Mesh::initGL(false);

    if( vert_to_ptcl.empty() ) {
        buf_x_s = buf_pos;
        buf_pos = 0;
        poss.clear(); poss.shrink_to_fit();
        tris.clear(); tris.shrink_to_fit();
    }
    else {
        glGenBuffers(1, &buf_x_s);
        glBindBuffer(GL_ARRAY_BUFFER, buf_x_s);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*x_s.size(), x_s.data(), GL_STATIC_COPY);
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
    

    glGenBuffers(1, &buf_debug);
    glBindBuffer(GL_ARRAY_BUFFER, buf_debug);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*nr_ptcls, nullptr, GL_DYNAMIC_COPY);




    struct GpuConstraintDist {
        int idx_p;
        float ori_dist;
    };
    vector<GpuConstraintDist> gpuDistConstraints;
    gpuDistConstraints.reserve(c_stretchs.size()*2);
    vector<ivec2> constraintOffsets;
    constraintOffsets.reserve(nr_ptcls);



    // stretch 
    gpuDistConstraints.clear();
    constraintOffsets.clear();
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
        constraintOffsets.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_stretchs);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_stretchs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_stretch_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_stretch_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*constraintOffsets.size(), constraintOffsets.data(), GL_STATIC_DRAW );



    // shear 
    gpuDistConstraints.clear();
    constraintOffsets.clear();
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
        constraintOffsets.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_shears);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_shears);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_shear_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_shear_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*constraintOffsets.size(), constraintOffsets.data(), GL_STATIC_DRAW );




    // bend 
    gpuDistConstraints.clear();
    constraintOffsets.clear();
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
        constraintOffsets.push_back(ivec2{sOffset, eOffset});
    }
    glGenBuffers(1, &buf_c_dist_bends);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_dist_bends);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GpuConstraintDist)*gpuDistConstraints.size(), gpuDistConstraints.data(), GL_STATIC_DRAW );
    glGenBuffers(1, &buf_c_dist_bend_offsets);
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_dist_bend_offsets);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ivec2)*constraintOffsets.size(), constraintOffsets.data(), GL_STATIC_DRAW );



    // Todo: dih bend, iso bend ...   
}



void SoftBodyGpu::deinitGL()
{
    lim::gl::safeDelBufs(&buf_x_s); // 0
    lim::gl::safeDelBufs(&buf_p_s); // 1
    lim::gl::safeDelBufs(&buf_v_s); // 2
    lim::gl::safeDelBufs(&buf_w_s); // 3
    // lim::gl::safeDelBufs(&buf_f_s);

    lim::gl::safeDelBufs(&buf_c_stretchs); // 4
    lim::gl::safeDelBufs(&buf_c_stretch_offsets); // 5
    lim::gl::safeDelBufs(&buf_c_shears); // 6
    lim::gl::safeDelBufs(&buf_c_shear_offsets); // 7
    lim::gl::safeDelBufs(&buf_c_dist_bends); // 8
    lim::gl::safeDelBufs(&buf_c_dist_bend_offsets); // 9
    lim::gl::safeDelBufs(&buf_debug); // 9

    Mesh::deinitGL();
}