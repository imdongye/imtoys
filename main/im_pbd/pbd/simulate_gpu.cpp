#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <limbrary/gl_tools.h>
#include <glm/gtx/norm.hpp>
#include <limbrary/log.h>

using namespace glm;
using namespace pbd;
using std::vector;


PhySceneGpu::PhySceneGpu()
{
    prog_update_p_s.attatch("im_pbd/shaders/simulate/update_p_s.comp").link();
    prog_project_dist.attatch("im_pbd/shaders/simulate/project_dist.comp").link();
    prog_update_x_s.attatch("im_pbd/shaders/simulate/update_x_s.comp").link();
    prog_update_verts.attatch("im_pbd/shaders/simulate/update_verts.comp").link();
    prog_apply_collision.attatch("im_pbd/shaders/simulate/apply_collision.comp").link();
}

void SoftBodyGpu::update( float dt, const PhySceneGpu& scene )
{
    float subDt = dt/nr_steps;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_x_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_p_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_v_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_w_s);

	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_c_stretchs);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_stretch_offsets);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_shears);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_c_shear_offsets);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, buf_c_dist_bends);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, buf_c_dist_bend_offsets);
	

    for( int step=0; step<nr_steps; step++ )
    {
        // update v and p (external force)
        {
            const lim::Program& prog = scene.prog_update_p_s;
            prog.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("gravity", scene.G);
            prog.setUniform("air_drag", scene.air_drag);
            prog.setUniform("inv_body_mass", inv_body_mass);
            prog.setUniform("dt", subDt);
            glDispatchCompute(nr_thread_groups, 1, 1);
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        }
    
        // project constraints



        // update v and x
        {
            const lim::Program& prog = scene.prog_update_x_s;
            prog.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("dt", subDt);
            glDispatchCompute(nr_thread_groups, 1, 1);
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        }



        // apply collision
        {
            const lim::Program& prog = scene.prog_apply_collision;
            prog.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("dt", subDt);
            glDispatchCompute(nr_thread_groups, 1, 1);
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        }
    }
    // update verts

}

void PhySceneGpu::update( float dt )
{
    for( auto body : bodies )
    {
        body->update( dt, *this );
    }
}