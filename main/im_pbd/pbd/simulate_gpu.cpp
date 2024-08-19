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
    prog_pbd.attatch(                       "im_pbd/shaders/simulate/pbd.comp").link();
    prog_0_update_p_s.attatch(              "im_pbd/shaders/simulate/0_update_p_s.comp").link();
    prog_1_project_dih_bend.attatch(        "im_pbd/shaders/simulate/1_project_dih_bend.comp").link();
    prog_1_project_dist.attatch(            "im_pbd/shaders/simulate/1_project_dist.comp").link();
    prog_1_project_point.attatch(           "im_pbd/shaders/simulate/1_project_point.comp").link();
    prog_2_update_x_s.attatch(              "im_pbd/shaders/simulate/2_update_x_s.comp").link();
    prog_3_apply_collision.attatch(         "im_pbd/shaders/simulate/3_apply_collision.comp").link();
    prog_3_apply_pressure_impulse.attatch(  "im_pbd/shaders/simulate/3_apply_pressure_impulse.comp").link();
    prog_4_make_ptcl_nors.attatch(          "im_pbd/shaders/simulate/4_make_ptcl_nors.comp").link();
    prog_4_update_vert_poss_nors.attatch(   "im_pbd/shaders/simulate/4_update_vert_poss_nors.comp").link();
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
// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, buf_debugs);
	

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


//     glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
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
    float dihBendAlpha = params.inv_stiff_dih_bend / sqSubDt;
    float distAlpha = params.inv_stiff_dist / sqSubDt;
    float stretchAlpha = distAlpha / params.stretch_pct;
    float shearAlpha = distAlpha / params.shear_pct;
    float bendAlpha = distAlpha / params.bend_pct;
    float pointAlpha = params.inv_stiff_point / sqSubDt;


    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_x_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_p_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_v_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_w_s);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_debugs);

    for( int step=0; step<nr_steps; step++ )
    {
        // update v and p (external force)
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_0_update_p_s.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("gravity", scene.G);
            prog.setUniform("air_drag", scene.air_drag);
            prog.setUniform("inv_body_mass", inv_body_mass);
            prog.setUniform("dt", subDt);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }



        // project dihedral bend constraints
        if( buf_c_dih_bends!=0 ) {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_1_project_dih_bend.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_dih_bend_idx_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_dih_bend_idxs);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_c_dih_bends);
            prog.setUniform("alpha", dihBendAlpha);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }

        // project distance constraints
        {
            // dist_bend
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_1_project_dist.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_dist_bends);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_dist_bend_offsets);
            prog.setUniform("alpha", bendAlpha);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);

            // shear
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_shears);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_shear_offsets);
            prog.setUniform("alpha", shearAlpha);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);

            // stretch
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_stretchs);
	        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_c_stretch_offsets);
            prog.setUniform("alpha", stretchAlpha);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }
        // project point constraints
        if( buf_c_points!=0 ) {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_c_points);
            const lim::Program& prog = scene.prog_1_project_point.use();
            prog.setUniform("alpha", pointAlpha);
            glDispatchCompute(c_points.size(), 1, 1);
        }
        


        // update v and x (verlet integration)
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_2_update_x_s.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("inv_dt", invSubDt);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }

        // apply collision
        {
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            const lim::Program& prog = scene.prog_3_apply_collision.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }

        lim::gl::errorAssert();
    } // end substeps




    // applyPressureImpulse
    if( params.pressure > glim::feps ) {
        downloadXs(); // Todo: update volume in gpu
        c_volume.cur_six_volume = getVolumeTimesSix();
        if( c_volume.cur_six_volume > glim::feps ) {
            float coeff = params.pressure*dt/c_volume.cur_six_volume;
            glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_adj_tri_idx_offsets);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_adj_tri_idxs);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_ptcl_tris);
            const lim::Program& prog = scene.prog_3_apply_pressure_impulse.use();
            prog.setUniform("nr_ptcls", nr_ptcls);
            prog.setUniform("coeff", coeff);
            glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
        }
    }


    // update verts
    if( buf_vert_to_ptcl==0 ) {
        glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_nors); // update vertex normals
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_adj_tri_idx_offsets);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_adj_tri_idxs);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_ptcl_tris);
        const lim::Program& prog = scene.prog_4_make_ptcl_nors.use();
        prog.setUniform("nr_ptcls", nr_ptcls);
        glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);
    }
    else {
        glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        // binding=1 : ps // make ptcl nors to p_s(temp storage);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_adj_tri_idx_offsets);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_adj_tri_idxs);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_ptcl_tris);
        const lim::Program& progA = scene.prog_4_make_ptcl_nors.use();
        progA.setUniform("nr_ptcls", nr_ptcls);
        glDispatchCompute(nr_thread_groups_by_ptcls, 1, 1);

        glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, buf_poss);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, buf_nors);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, buf_vert_to_ptcl);
        const lim::Program& progB = scene.prog_4_update_vert_poss_nors.use();
        progB.setUniform("nr_verts", nr_verts);
        glDispatchCompute(nr_thread_groups_by_verts, 1, 1);
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