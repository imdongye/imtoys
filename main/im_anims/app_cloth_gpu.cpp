/*
	imdongye / 2024.07.10
	Fast Simulation of Mass-Spring Systems
	From: http://graphics.berkeley.edu/papers/Liu-FSM-2013-11/Liu-FSM-2013-11.pdf
	
*/

#include "app_cloth_gpu.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/program.h>
#include <limbrary/asset_lib.h>
#include <limbrary/limgui.h>
#include <limbrary/gl_tools.h>

using namespace lim;
using namespace glm;

#include "app_cloth_coef.h"

namespace {
	GLuint xfb_id;
	GLuint vao_update_ids[2];
	GLuint vao_render_ids[2];
	GLuint vbo_posm_ids[2];
	GLuint vbo_vel_ids[2];
	GLuint ebo_indices;
	GLuint tbo_pos_ids[2];
	GLuint tbo_vel_ids[2];
	int nr_ptcls, nr_tris;

	int src_buf_idx = 0;
	int dst_buf_idx = 1;
	int picked_ptcl_idx = -1;
	constexpr int invocations_width = 16;
}
static void clearGLBuffers() {
	gl::safeDelXfbs(&xfb_id);
	gl::safeDelVertArrs(vao_update_ids, 2);
	gl::safeDelVertArrs(vao_render_ids, 2);
	gl::safeDelBufs(vbo_posm_ids, 2);
	gl::safeDelBufs(vbo_vel_ids, 2);
	gl::safeDelBufs(&ebo_indices);
	gl::safeDelTexs(tbo_pos_ids, 2);
	gl::safeDelTexs(tbo_vel_ids, 2);
}

namespace {
	std::vector<vec4> cloth_pm_data;
	std::vector<vec4> cloth_v_data;

	bool collision_enabled = false;

	bool is_rendered_frame = true; // for step debugging
}

static void copyMemToBuf() {
	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_update_ids[i]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[i]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4)*nr_ptcls, cloth_pm_data.data());

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vel_ids[i]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4)*nr_ptcls, cloth_v_data.data());
		glBindVertexArray(0);
	}
}

static void resetParams() {
	// nr_p = {10, 20};
	// cloth_size = {0.7, 1.4};


	time_speed = def_time_speed;
	// step_size = def_step_size;
	step_size = 30;
	Ka = def_Ka; 	
	Kr = def_Kr;
	Kmu = def_Kmu;
	Ks = def_Ks; 		
	Kd = def_Kd;
	
	is_pause = true;
	cloth_mass = def_cloth_m;
	ptcl_mass = cloth_mass/(nr_p.x*nr_p.y);
	stretch_pct = def_stretch_pct;
	shear_pct = def_shear_pct;
	bending_pct = def_bending_pct;
}

static void makeClothDataAndInitGL() {
	clearGLBuffers();

	inter_p_size = cloth_size/vec2{nr_p.x-1, nr_p.y-1};
	ptcl_mass = cloth_mass/(nr_p.x*nr_p.y);

	Transform ctf;
	ctf.pos = {0,2,0};
	ctf.ori = quat(rotate(H_PI*0.0f, vec3{1,0,0}));
	ctf.scale = vec3(cloth_size.x, 1, cloth_size.y);
	ctf.scale *= 0.5f;
	ctf.update();

	MeshPlane plane(1, nr_p.x-1, nr_p.y-1, false, false);
	nr_ptcls = plane.poss.size();
	cloth_pm_data.resize(nr_ptcls);
	for(int i=0; i<nr_ptcls; i++) {
		vec3 wPos = vec3(ctf.mtx*vec4(plane.poss[i],1));
		cloth_pm_data[i] = vec4(wPos, 1.f);
	}

	// fix first and last in first row
	cloth_pm_data[0].w = 0.f;
	cloth_pm_data[nr_p.x-1].w = 0.f;

	cloth_v_data.resize(nr_ptcls, vec4(0));


	glGenVertexArrays(2, vao_update_ids);
	glGenVertexArrays(2, vao_render_ids);
	glGenBuffers(2, vbo_posm_ids);
	glGenBuffers(2, vbo_vel_ids);
	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_update_ids[i]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*nr_ptcls, nullptr, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vel_ids[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*nr_ptcls, nullptr, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindVertexArray(0);
	}

	glGenTextures(2, tbo_pos_ids);
	glGenTextures(2, tbo_vel_ids);
	for(int i=0; i<2; i++) {
		glBindTexture(GL_TEXTURE_BUFFER, tbo_pos_ids[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_posm_ids[i]);

		glBindTexture(GL_TEXTURE_BUFFER, tbo_vel_ids[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_vel_ids[i]);

		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}


	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_render_ids[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[i]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindVertexArray(0);
	}

	nr_tris = plane.tris.size();
	glGenBuffers(1, &ebo_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uvec3)*nr_tris, plane.tris.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	copyMemToBuf();

	glGenTransformFeedbacks(1, &xfb_id);
}

AppClothGPU::AppClothGPU()
	: AppBase(900, 600, APP_NAME, false)
	, viewport("viewport##gpgpu", new FramebufferTexDepth())
	, ground(20)
{
	viewport.camera.pivot = vec3(0, 1.0, 0);
    viewport.camera.pos = vec3(0, 1.5, 3.4);
	viewport.camera.updateViewMat();

	resetParams();
	makeClothDataAndInitGL();

	prog.attatch("im_anims/shaders/cloth.vs").attatch("im_anims/shaders/cloth.fs").link();
	prog_xfb.attatch("im_anims/shaders/cloth_xfb.vs").link();

	prog_comp.attatch("im_anims/shaders/cloth.comp").link();

	model.importFromFile("assets/models/jump.fbx", true);
	model.setUnitScaleAndPivot();
	model.tf->pos.y += model.pivoted_scaled_bottom_height;
	model.tf->update();
	model.animator.setTimeline(0.5f, true);
}
AppClothGPU::~AppClothGPU()
{
	clearGLBuffers();
}
void AppClothGPU::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bool frameByFrame = is_pause && (is_rendered_frame==false);
	if(!is_pause || frameByFrame)
	{
		is_rendered_frame = true;


		// prog_xfb.use();
		// float dt = (delta_time*time_speed)/float(step_size);
		// prog_xfb.setUniform("cloth_mass", cloth_mass);
		// prog_xfb.setUniform("ptcl_mass", ptcl_mass);
		// prog_xfb.setUniform("inter_p_size", inter_p_size);
		// prog_xfb.setUniform("nr_p", nr_p);
		// prog_xfb.setUniform("dt", dt);
		// prog_xfb.setUniform("ka", Ka);
		// prog_xfb.setUniform("kr", Kr);
		// prog_xfb.setUniform("stretchKs", 	stretch_pct*Ks);
		// prog_xfb.setUniform("shearKs", 		shear_pct*Ks);
		// prog_xfb.setUniform("bendingKs", 	bending_pct*Ks);
		// prog_xfb.setUniform("stretchKd", 	Kd);
		// prog_xfb.setUniform("shearKd", 		Kd);
		// prog_xfb.setUniform("bendingKd", 	Kd);
		// prog_xfb.setUniform("gravity", G);

		// prog_xfb.setUniform("tex_posm", 0);
		// prog_xfb.setUniform("tex_vel", 1);

		// prog_xfb.setUniform("collision_enabled", collision_enabled);

		// glEnable(GL_RASTERIZER_DISCARD);
		// {
		// 	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb_id);
		// 	{
		// 		for(int i=0; i<step_size; i++)
		// 		{
		// 			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo_posm_ids[dst_buf_idx]);
		// 			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vbo_vel_ids[dst_buf_idx]);

		// 			glActiveTexture(GL_TEXTURE0);
		// 			glBindTexture(GL_TEXTURE_BUFFER, tbo_pos_ids[src_buf_idx]);
		// 			glActiveTexture(GL_TEXTURE1);
		// 			glBindTexture(GL_TEXTURE_BUFFER, tbo_vel_ids[src_buf_idx]);

		// 			glBeginTransformFeedback(GL_POINTS);
		// 			{
		// 				glBindVertexArray(vao_update_ids[src_buf_idx]);
		// 				glDrawArrays(GL_POINTS, 0, nr_ptcls);
		// 			}
		// 			glEndTransformFeedback();
		// 			std::swap(src_buf_idx, dst_buf_idx);
		// 		}
		// 	}
		// 	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		// }
		// glDisable(GL_RASTERIZER_DISCARD);
		// glFlush();

		prog_comp.use();
		// float dt = (delta_time*time_speed)/float(step_size);
		float dt = (delta_time*time_speed);
		prog_comp.setUniform("cloth_mass", cloth_mass);
		prog_comp.setUniform("ptcl_mass", ptcl_mass);
		prog_comp.setUniform("inter_p_size", inter_p_size);
		prog_comp.setUniform("nr_p", nr_p);
		prog_comp.setUniform("dt", dt);
		prog_comp.setUniform("ka", Ka);
		prog_comp.setUniform("kr", Kr);
		prog_comp.setUniform("stretchKs", 	stretch_pct*Ks);
		prog_comp.setUniform("shearKs", 		shear_pct*Ks);
		prog_comp.setUniform("bendingKs", 	bending_pct*Ks);
		prog_comp.setUniform("stretchKd", 	Kd);
		prog_comp.setUniform("shearKd", 		Kd);
		prog_comp.setUniform("bendingKd", 	Kd);
		prog_comp.setUniform("gravity", G);

		prog_comp.setUniform("tex_posm", 0);
		prog_comp.setUniform("tex_vel", 1);

		prog_comp.setUniform("collision_enabled", collision_enabled);

		prog_comp.setUniform("step_size", step_size);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_posm_ids[src_buf_idx]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_vel_ids[src_buf_idx]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_posm_ids[dst_buf_idx]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_vel_ids[dst_buf_idx]);
		glDispatchCompute(fastIntCeil(nr_p.x, invocations_width), fastIntCeil(nr_p.y, invocations_width), 1);
		glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );

		// for(int i=0; i<step_size; i++)
		// {
		// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_posm_ids[src_buf_idx]);
		// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_vel_ids[src_buf_idx]);
		// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_posm_ids[dst_buf_idx]);
		// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vbo_vel_ids[dst_buf_idx]);
		// 	glDispatchCompute(fastIntCeil(nr_p.x, invocations_width), fastIntCeil(nr_p.y, invocations_width), 1);
		// 	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
		// 	std::swap(src_buf_idx, dst_buf_idx);
		// }
	}


	viewport.getFb().bind();
	prog.use();
	viewport.camera.setUniformTo(prog);
	// prog.setUniform("mtx_Model", tf.mtx);
	// glBindVertexArray(vao_render_ids[src_buf_idx]);
	glBindVertexArray(vao_render_ids[dst_buf_idx]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_indices);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, nr_tris*3, GL_UNSIGNED_INT, nullptr);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	Program& ndvProg = AssetLib::get().ndv_prog;
	ndvProg.use();
	viewport.camera.setUniformTo(ndvProg);
	
	ndvProg.setUniform("mtx_Model", mat4(1));
	ground.bindAndDrawGL();

	if(collision_enabled) {
		// ndvProg.setUniform("mtx_Model", mtx_geo_model);
		// ori_geo_ms->bindAndDrawGL();
	}
	viewport.getFb().unbind();
}

void AppClothGPU::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	log::drawViewer("logger##gpgpu");
	viewport.drawImGui();



	ImGui::Begin("test window##gpgpu");
	if(ImGui::Button("release fixed ptcl")) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[src_buf_idx]);
		float* ptr = (float*)glMapBufferRange(GL_ARRAY_BUFFER, sizeof(vec3), sizeof(float),  GL_MAP_WRITE_BIT);
		log::glError();
		float aaa = 1.f;
		memcpy(ptr, &aaa, sizeof(float));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	ImGui::Checkbox("collision enabled", &collision_enabled);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	if(ImGui::Button("restart")) {
		copyMemToBuf();
	}
	if(ImGui::Button("reset params")) {
		resetParams();
	}
	ImGui::Checkbox("pause", &is_pause);

	ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	ImGui::SliderInt("step size", &step_size, 1, 60);

	ImGui::SliderFloat("plane bounce damping", &Kr, 0.01f, 1.f);
	ImGui::SliderFloat("plane friction", &Kmu, 0.01f, 1.f);

	ImGui::SliderFloat("air damping", &Ka, 0.0001f, 0.9f, "%.5f");

	if(ImGui::SliderFloat("cloth mass", &cloth_mass, 0.05f, 0.7f)) {
		ptcl_mass = cloth_mass/(nr_p.x*nr_p.y);
	}
	ImGui::SliderFloat("spring coef", &Ks, 10.f, 70.f);
	ImGui::SliderFloat("spring damping coef", &Kd, 0.00f, 1.4f);
	ImGui::SliderFloat("stretch", &stretch_pct, 0.1f, 1.f);
	ImGui::SliderFloat("shear", &shear_pct, 0.1f, 1.f);
	ImGui::SliderFloat("bending", &bending_pct, 0.1f, 1.f);

	// if(ImGui::SliderInt2("nr cloth ptcls", (int*)&nr_p, 2, 200)) {
	// 	makeClothData();
	// }
	if(ImGui::SliderInt("nr cloth ptcls", (int*)&nr_p.x, 2, 200)) {
		nr_p.y = nr_p.x;
		makeClothDataAndInitGL();
	}
	
	if(ImGui::Button("step one frame")) {
		is_rendered_frame = false;
	}
	ImGui::End();

	
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false))
	{
		const int nrPms = cloth_pm_data.size();
		const vec3 mouseRay = viewport.getMousePosRayDir();
		const vec3 cameraPos = viewport.camera.pos;
		float minDepth = FLT_MAX;

		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[src_buf_idx]);
		vec4* posms = (vec4*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		for( int i=0; i<nrPms; i++ ) {
			const vec3 objP = vec3(posms[i]);
			vec3 toObj = objP - cameraPos;
			float distFromLine = glm::length( glm::cross(mouseRay, toObj) );
			float distProjLine = glm::dot(mouseRay, toObj);

			if( distFromLine < 0.01f ) {
				if( distProjLine>0 && minDepth>distProjLine ) {
					minDepth = distProjLine;
					picked_ptcl_idx  = i;
				}
			}
		}
		if(picked_ptcl_idx>=0) {
			vec4* posm = &posms[picked_ptcl_idx];
			if( posm->w==0.f ) {
				posm->w = 1.f;
				picked_ptcl_idx = -1;
			}
			else {
				posm->w = 0.f;
			}
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	else if(picked_ptcl_idx>=0 && ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo_posm_ids[src_buf_idx]);
		vec3 objPos = *(vec3*)glMapBufferRange(GL_ARRAY_BUFFER, sizeof(vec4)*picked_ptcl_idx, sizeof(vec3), GL_MAP_READ_BIT); 
		glUnmapBuffer(GL_ARRAY_BUFFER);
		const vec3 toObj = objPos-viewport.camera.pos;
		const vec3 mouseRay = viewport.getMousePosRayDir();
		const float depth = dot(viewport.camera.front, toObj)/dot(viewport.camera.front, mouseRay);
		objPos = depth*mouseRay+viewport.camera.pos;
		glUnmapBuffer(GL_ARRAY_BUFFER);
		vec3* pObjPos = (vec3*)glMapBufferRange(GL_ARRAY_BUFFER, sizeof(vec4)*picked_ptcl_idx, sizeof(vec3), GL_MAP_WRITE_BIT); 
		*pObjPos = objPos;
		glUnmapBuffer(GL_ARRAY_BUFFER);
	} 
	else if( picked_ptcl_idx>=0 && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		picked_ptcl_idx = -1;
	}
}