#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <limbrary/gl_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using namespace pbd;
using std::vector;



SoftBodyGpu::SoftBodyGpu(const lim::Mesh& src, int nrShear, BendType bendType, float totalMass)
    : SoftBody(src, nrShear, bendType, totalMass)
{
    vector<vec4> tempBuf;
    tempBuf.reserve(2*nr_ptcls*nr_ptcls);


    glGenBuffers(1, &buf_xw_s);
    for(int i=0; i<nr_ptcls; i++) {
        tempBuf.push_back(vec4{poss[i], w_s[i]});
    }
    glBindBuffer(GL_ARRAY_BUFFER, buf_xw_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*tempBuf.size(), tempBuf.data(), GL_DYNAMIC_COPY);


    glGenBuffers(1, &buf_pw_s);
    for(int i=0; i<nr_ptcls; i++) {
        tempBuf.push_back(vec4{p_s[i], w_s[i]});
    }
    glBindBuffer(GL_ARRAY_BUFFER, buf_pw_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*tempBuf.size(), tempBuf.data(), GL_DYNAMIC_COPY);


    glGenBuffers(1, &buf_v_s);
    for(vec3& v: v_s) {
        tempBuf.push_back(vec4{v, 0.f});
    }
    glBindBuffer(GL_ARRAY_BUFFER, buf_v_s);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*tempBuf.size(), tempBuf.data(), GL_DYNAMIC_COPY);


    glGenBuffers(1, &buf_c_stretchs);
    for(auto& c: c_stretchs) {
        vec4 pack;
        pack.x = c.idx_ps.x;
        pack.y = c.idx_ps.y;
        pack.z = c.ori_dist;
        tempBuf.push_back(pack);
    }
    glBindBuffer(GL_ARRAY_BUFFER, buf_c_stretchs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*tempBuf.size(), tempBuf.data(), GL_DYNAMIC_COPY);
}

SoftBodyGpu::~SoftBodyGpu()
{
    lim::gl::safeDelBufs(&buf_xw_s);
    lim::gl::safeDelBufs(&buf_pw_s);
    lim::gl::safeDelBufs(&buf_v_s);
    lim::gl::safeDelBufs(&buf_f_s);

    lim::gl::safeDelBufs(&buf_c_stretchs);
    lim::gl::safeDelBufs(&buf_c_shears);
    lim::gl::safeDelBufs(&buf_c_dist_bends);

    lim::gl::safeDelBufs(&buf_c_dih_bends);
    lim::gl::safeDelBufs(&buf_c_iso_bends);
    lim::gl::safeDelBufs(&buf_c_g_volumes);

    lim::gl::safeDelVertArrs(&vao_soft_body);
}

void SimulatorGpu::update( float dt )
{
    
}