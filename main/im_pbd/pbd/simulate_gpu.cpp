#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <limbrary/gl_tools.h>
#include <glm/gtx/norm.hpp>
#include <limbrary/log.h>
#include <limbrary/gl_tools.h>

using namespace glm;
using namespace pbd;
using std::vector;


PhySceneGpu::PhySceneGpu()
{
    prog_pbd.attatch(            "im_pbd/shaders/simulate/pbd.comp").link();
    prog_update_p_s.attatch(     "im_pbd/shaders/simulate/0_update_p_s.comp").link();
    prog_project_dist.attatch(   "im_pbd/shaders/simulate/1_project_dist.comp").link();
    prog_update_x_s.attatch(     "im_pbd/shaders/simulate/2_update_x_s.comp").link();
    prog_apply_collision.attatch("im_pbd/shaders/simulate/3_apply_collision.comp").link();
    prog_update_verts.attatch(   "im_pbd/shaders/simulate/4_update_verts.comp").link();
}

// void SoftBodyGpu::update( float dt, const PhySceneGpu& scene )
// {
//     float subDt = dt/nr_steps;

//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_x_s);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_p_s);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_v_s);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_w_s);

// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_c_stretchs);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_stretch_offsets);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_shears);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_c_shear_offsets);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buf_c_dist_bends);
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, buf_c_dist_bend_offsets);
	

//     for( int step=0; step<nr_steps; step++ )
//     {

//         // update v and p (external force)
//         {
//             glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
//             const lim::Program& prog = scene.prog_update_p_s;
//             prog.use();
//             prog.setUniform("nr_ptcls", nr_ptcls);
//             prog.setUniform("gravity", scene.G);
//             prog.setUniform("air_drag", scene.air_drag);
//             prog.setUniform("inv_body_mass", inv_body_mass);
//             prog.setUniform("dt", subDt);
//             glDispatchCompute(nr_thread_groups, 1, 1);
//         }

//         // project constraints
        
        
//         // update v and x
//         {
//             glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
//             const lim::Program& prog = scene.prog_update_x_s;
//             prog.use();
//             prog.setUniform("nr_ptcls", nr_ptcls);
//             prog.setUniform("dt", subDt);
//             glDispatchCompute(nr_thread_groups, 1, 1);
//         }

//         // apply collision
//         {
//             glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
//             const lim::Program& prog = scene.prog_apply_collision;
//             prog.use();
//             prog.setUniform("nr_ptcls", nr_ptcls);
//             prog.setUniform("dt", subDt);
//             glDispatchCompute(nr_thread_groups, 1, 1);
//         }

//         lim::gl::errorAssert();
//     }
//     // update verts


//     glMemoryBarrier( GL_ALL_BARRIER_BITS );
// }


void SoftBodyGpu::update( float dt, const PhySceneGpu& scene )
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_x_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_p_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_v_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_w_s);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_c_stretchs);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_stretch_offsets);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_shears);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_c_shear_offsets);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buf_c_dist_bends);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, buf_c_dist_bend_offsets);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, buf_debug);
	

    float subDt = dt/nr_steps;;
    float sqSubDt = subDt*subDt;
    float alpha_stretch = params.inv_stiff_dist/sqSubDt;

    glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
    const lim::Program& prog = scene.prog_pbd;
    prog.use();
    prog.setUniform("nr_ptcls", nr_ptcls);
    prog.setUniform("gravity", scene.G);
    prog.setUniform("air_drag", scene.air_drag);
    prog.setUniform("inv_body_mass", inv_body_mass);
    prog.setUniform("nr_steps", nr_steps);

    prog.setUniform("dt", dt);
    prog.setUniform("subDt", subDt);

    prog.setUniform("alpha_stretch", alpha_stretch);


    glDispatchCompute(nr_thread_groups, 1, 1);
    glMemoryBarrier( GL_ALL_BARRIER_BITS );

    lim::gl::errorAssert();
}

void PhySceneGpu::update( float dt )
{
    for( auto body : bodies )
    {
        body->update( dt, *this );
    }
}