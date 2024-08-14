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
    prog_update_nors_with_ptcl.attatch(   "im_pbd/shaders/simulate/4_update_nors_with_ptcl.comp").link();
}



// combine version, barrier in shader ========================================
// void SoftBodyGpu::update( float dt, const PhySceneGpu& scene )
// {
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
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, buf_debug);
	

//     float subDt = dt/nr_steps;;
//     float sqSubDt = subDt*subDt;
//     float alpha_stretch = params.inv_stiff_dist/(sqSubDt*params.stretch_pct);

//     glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
//     const lim::Program& prog = scene.prog_pbd;
//     prog.use();
//     prog.setUniform("nr_ptcls", nr_ptcls);
//     prog.setUniform("gravity", scene.G);
//     prog.setUniform("air_drag", scene.air_drag);
//     prog.setUniform("inv_body_mass", inv_body_mass);
//     prog.setUniform("nr_steps", nr_steps);

//     prog.setUniform("dt", dt);
//     prog.setUniform("subDt", subDt);

//     prog.setUniform("alpha_stretch", alpha_stretch);


//     glDispatchCompute(nr_thread_groups, 1, 1);
//     glMemoryBarrier( GL_ALL_BARRIER_BITS );

//     lim::gl::errorAssert();
// }



// separate dispatch by update p_s and x_s version, with api level barrier  ========================
void SoftBodyGpu::update( float dt, const PhySceneGpu& scene )
{
    nr_steps = min(int(dt/0.0001f), nr_steps);
    float subDt = dt/nr_steps;
    float invSubDt = 1.f/subDt;
    float sqSubDt = subDt*subDt;
    float distAlpha = params.inv_stiff_dist / sqSubDt;
    float stretchAlpha = distAlpha / params.stretch_pct;
    float shearAlpha = distAlpha / params.shear_pct;
    float bendAlpha = distAlpha / params.bend_pct;


    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_x_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_p_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_v_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_w_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_debug);

    for( int step=0; step<nr_steps; step++ )
    {
        // update v and p (external force)
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_update_p_s.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("gravity", scene.G);
            prog.setUniform("air_drag", scene.air_drag);
            prog.setUniform("inv_body_mass", inv_body_mass);
            prog.setUniform("dt", subDt);
            glDispatchCompute(nr_thread_groups, 1, 1);
        }

        // project constraints
        {
            // dist_bend
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_project_dist.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_dist_bends);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_dist_bend_offsets);
            prog.setUniform("alpha", bendAlpha);
            glDispatchCompute(nr_thread_groups, 1, 1);

            // shear
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_shears);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_shear_offsets);
            prog.setUniform("alpha", shearAlpha);
            glDispatchCompute(nr_thread_groups, 1, 1);

            // stretch
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_stretchs);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_stretch_offsets);
            prog.setUniform("alpha", stretchAlpha);
            glDispatchCompute(nr_thread_groups, 1, 1);
        }
        
        // update v and x (verlet integration)
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_update_x_s.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("inv_dt", invSubDt);
            glDispatchCompute(nr_thread_groups, 1, 1);
        }

        // apply collision
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_apply_collision.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            glDispatchCompute(nr_thread_groups, 1, 1);
        }

        lim::gl::errorAssert();
    }

    // update verts
    {
        glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_nors);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_ptcl_tris);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_adj_tri_idxs);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_adj_tri_idx_offsets);
        const lim::Program& prog = scene.prog_update_nors_with_ptcl.use();
        prog.setUniform("nr_ptcls", nr_ptcls);
        glDispatchCompute(nr_thread_groups, 1, 1);
    }

    glMemoryBarrier( GL_ALL_BARRIER_BITS );
}


void PhySceneGpu::update( float dt )
{
    for( auto body : bodies )
    {
        body->update( dt, *this );
    }
}