#include "app_gpgpu.h"
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

// ! test xfb
// AppGpgpu::AppGpgpu()
// 	: AppBase(1280, 720, "gpgpu")
// 	, plane(1, 20)
// 	, viewport("viewport##gpgpu", new FramebufferTexDepth())
// {
// 	uint pos_vbo, pos_vao;
// 	vec3 pos_data[10] = {
// 		{0, 1, 0},
// 		{0, 1, 1},
// 		{1, 1, 0},
// 		{1, 1, 1},
// 		{0, 0, 1},

// 		{1, 1, 0},
// 		{1, 1, 0},
// 		{0, 1, 2},
// 		{0, 1, 0},
// 		{0, 1, 3},
// 	};

// 	Program xfb_prog;
// 	uint xfb;
// 	uint xfb_out_buf;

// 	float out_length[10];
// 	vec3 out_norm[10];

// 	glGenVertexArrays(1, &pos_vao);
// 	glBindVertexArray(pos_vao);

// 	glGenBuffers(1, &pos_vbo);
// 	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
// 	glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);
// 	glEnableVertexAttribArray(0);
// 	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

// 	glGenBuffers(1, &xfb_out_buf);
// 	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
// 	glBufferData(GL_ARRAY_BUFFER, sizeof(out_length)+sizeof(out_norm), nullptr, GL_DYNAMIC_COPY);

// 	xfb_prog.attatch("im_tests/shaders/xfb.vs").link();

// 	glGenTransformFeedbacks(1, &xfb);
// 	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
// 	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_out_buf, 0, sizeof(out_length));
// 	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 1, xfb_out_buf, sizeof(out_length), sizeof(out_norm));


// 	xfb_prog.use();
// 	glEnable(GL_RASTERIZER_DISCARD);
// 	{
// 		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
// 		{
// 			glBeginTransformFeedback(GL_POINTS);
// 			{
// 				glBindVertexArray(pos_vao);
// 				glDrawArrays(GL_POINTS, 0, 10);
// 			}
// 			glEndTransformFeedback();
// 		}
// 		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
// 	}
// 	glDisable(GL_RASTERIZER_DISCARD);
// 	glFlush();


// 	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
// 	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(out_length), &out_length);
// 	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
// 	glGetBufferSubData(GL_ARRAY_BUFFER, sizeof(out_length), sizeof(out_norm), &out_norm);

// 	for(int i=0; i<sizeof(out_norm)/sizeof(out_norm[0]); i++)
// 		log::pure("%f %f %f\n", out_norm[i].x, out_norm[i].y, out_norm[i].z);
// 	for(int i=0; i<sizeof(out_length)/sizeof(out_length[0]); i++)
// 		log::pure("%f\n", out_length[i]);

// 	glDeleteBuffers(1, &pos_vbo);
// 	glDeleteTransformFeedbacks(1, &xfb);
// 	glDeleteBuffers(1, &xfb_out_buf);
// }



namespace {
	GLuint xfb_id;
	GLuint vao_update_ids[2];
	GLuint vao_render_ids[2];
	GLuint vbo_pos_ids[2];
	GLuint vbo_vel_ids[2];
	GLuint vbo_indices;
	GLuint tbo_pos_ids[2];
	GLuint tbo_vel_ids[2];
	int nr_ptcls, nr_tris;
}
static void clearGLBuffers() {
	gl::safeDelXfbs(1, &xfb_id);
	gl::safeDelVertArrs(2, vao_update_ids);
	gl::safeDelVertArrs(2, vao_render_ids);
	gl::safeDelBufs(2, vbo_pos_ids);
	gl::safeDelBufs(2, vbo_vel_ids);
	gl::safeDelBufs(1, &vbo_indices);
	gl::safeDelTexs(2, tbo_pos_ids);
	gl::safeDelTexs(2, tbo_vel_ids);
}

namespace {
	constexpr float def_time_speed = 1.f;
	constexpr int 	def_step_size = 50;
	constexpr float def_Ka = 0.026f;	// vicous drag 공기저항
	constexpr float def_Kr = 0.8f;		// 반발 계수
	constexpr float def_Kmu = 0.1f; 	// 마찰 계수
	constexpr float def_Ks = 10.f;		// 스프링 계수
	constexpr float def_Kd = 0.014f;	// 스프링 저항 gpu에서 계산할때 왜 과하게 스프링 저항이 들어가는지 모르겠음
	constexpr float def_collision_eps = 0.001f; // (particle ratius)
	constexpr float def_stretch_pct = 1.f;
	constexpr float def_shear_pct = 0.9f;
	constexpr float def_bending_pct = 0.75f;
	constexpr float def_M = 0.01f; // 10g 질량


	ivec2 nr_p{7, 7};
	vec2 cloth_size{0.7, 0.7};
	vec2 inter_p_size;

	float time_speed = def_time_speed;
	int step_size = def_step_size;
	float Ka = def_Ka; 	
	float Kr = def_Kr;
	float Kmu = def_Kmu;
	float Ks = def_Ks; 		
	float Kd = def_Kd;
	
	bool is_pause = true;
	float cloth_p_mass = def_M;
	float stretch_pct = def_stretch_pct;
	float shear_pct = def_shear_pct;
	float bending_pct = def_bending_pct;

	const vec3 G = {0, -9.8, 0};
	std::vector<vec4> cloth_p_data;
	std::vector<vec4> cloth_v_data;

	bool is_rendered_frame = true;
}

static void resetScene() {
	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_update_ids[i]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_pos_ids[i]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4)*nr_ptcls, cloth_p_data.data());

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vel_ids[i]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4)*nr_ptcls, cloth_v_data.data());
		glBindVertexArray(0);
	}
}

static void resetParams() {
	time_speed = def_time_speed;
	step_size = def_step_size;
	Ka = def_Ka; 	
	Kr = def_Kr;
	Kmu = def_Kmu;
	Ks = def_Ks; 		
	Kd = def_Kd;
	
	is_pause = true;
	cloth_p_mass = def_M;
	stretch_pct = def_stretch_pct;
	shear_pct = def_shear_pct;
	bending_pct = def_bending_pct;
}

static void makeClothData() {
	clearGLBuffers();
	Transform ctf;
	ctf.pos = {0,2,0};
	ctf.ori = quat(rotate(H_PI*0.0f, vec3{1,0,0}));
	ctf.scale = vec3(cloth_size.x, 1, cloth_size.y);
	ctf.scale *= 0.5f;
	ctf.update();
	inter_p_size = cloth_size/vec2{nr_p.x-1, nr_p.y-1};
	MeshPlane plane(1, nr_p.x-1, nr_p.y-1);
	nr_ptcls = plane.poss.size();
	cloth_p_data.resize(nr_ptcls);
	for(int i=0; i<nr_ptcls; i++) {
		vec3 newPos = vec3(ctf.mtx*vec4(plane.poss[i],1));
		cloth_p_data[i] = vec4(newPos, 1.f);
	}
	cloth_p_data[0].w = 0.f;
	cloth_p_data[nr_p.x-1].w = 0.f;

	cloth_v_data.resize(nr_ptcls, vec4(0));


	glGenVertexArrays(2, vao_update_ids);
	glGenVertexArrays(2, vao_render_ids);
	glGenBuffers(2, vbo_pos_ids);
	glGenBuffers(2, vbo_vel_ids);
	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_update_ids[i]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_pos_ids[i]);
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
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_pos_ids[i]);

		glBindTexture(GL_TEXTURE_BUFFER, tbo_vel_ids[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_vel_ids[i]);

		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}


	for(int i=0; i<2 ; i++) {
		glBindVertexArray(vao_render_ids[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_pos_ids[i]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindVertexArray(0);
	}

	nr_tris = plane.tris.size();
	glGenBuffers(1, &vbo_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uvec3)*nr_tris, plane.tris.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenTransformFeedbacks(1, &xfb_id);

	resetScene();
}

AppGpgpu::AppGpgpu()
	: AppBase(1280, 720, "gpgpu")
	, viewport("viewport##gpgpu", new FramebufferTexDepth())
	, ground(20)
{
	makeClothData();

	viewport.camera.pivot = vec3(0, 1.0, 0);
    viewport.camera.pos = vec3(0, 1.5, 3.4);
	viewport.camera.updateViewMat();

	prog.attatch("im_tests/shaders/cloth.vs").attatch("im_tests/shaders/blue.fs").link();
	prog_xfb.attatch("im_tests/shaders/cloth_xfb.vs").link();
	
	makeClothData();
}
AppGpgpu::~AppGpgpu()
{
	
}
void AppGpgpu::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static int srcIdx=0;
	static int dstIdx=1;

	bool frameByFrame = is_pause && (is_rendered_frame==false);
	if(!is_pause || frameByFrame)
	{
		is_rendered_frame = true;
		prog_xfb.use();
		float dt = (delta_time*time_speed)/float(step_size);
		prog_xfb.setUniform("cloth_p_mass", cloth_p_mass);
		prog_xfb.setUniform("inter_p_size", inter_p_size);
		prog_xfb.setUniform("nr_p", nr_p);
		prog_xfb.setUniform("dt", dt);
		prog_xfb.setUniform("ka", Ka);
		prog_xfb.setUniform("kr", Kr);
		prog_xfb.setUniform("stretchKs", 	stretch_pct*Ks);
		prog_xfb.setUniform("shearKs", 		shear_pct*Ks);
		prog_xfb.setUniform("bendingKs", 	bending_pct*Ks);
		prog_xfb.setUniform("stretchKd", 	Kd);
		prog_xfb.setUniform("shearKd", 		Kd);
		prog_xfb.setUniform("bendingKd", 	Kd);
		prog_xfb.setUniform("gravity", G);

		glEnable(GL_RASTERIZER_DISCARD);
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb_id);
			{
				for(int i=0; i<step_size; i++)
				{
					glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo_pos_ids[dstIdx]);
					glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vbo_vel_ids[dstIdx]);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_BUFFER, tbo_pos_ids[srcIdx]);
					prog.setUniform("tex_posm", 0);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_BUFFER, tbo_vel_ids[srcIdx]);
					prog.setUniform("tex_vel", 1);

					glBeginTransformFeedback(GL_POINTS);
					{
						glBindVertexArray(vao_update_ids[srcIdx]);
						glDrawArrays(GL_POINTS, 0, nr_ptcls);
					}
					glEndTransformFeedback();
					std::swap(srcIdx, dstIdx);
				}
			}
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		}
		glDisable(GL_RASTERIZER_DISCARD);
		glFlush();
	}


	viewport.getFb().bind();
	prog.use();
	viewport.camera.setUniformTo(prog);
	// prog.setUniform("mtx_Model", tf.mtx);
	glBindVertexArray(vao_render_ids[dstIdx]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, nr_tris*3, GL_UNSIGNED_INT, nullptr);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	Program& groundProg = AssetLib::get().ndv_prog;
	groundProg.use();
	viewport.camera.setUniformTo(groundProg);
	groundProg.setUniform("mtx_Model", mat4(1));
	ground.bindAndDrawGL();
	viewport.getFb().unbind();
	std::swap(srcIdx, dstIdx);
}

void AppGpgpu::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##gpgpu");
	
	viewport.drawImGui();

	ImGui::Begin("test window##template");
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	if(ImGui::Button("restart")) {
		resetScene();
	}
	if(ImGui::Button("reset params")) {
		resetParams();
	}
	ImGui::Checkbox("pause", &is_pause);


	ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	ImGui::SliderInt("step size", &step_size, 1, 60);

	ImGui::SliderFloat("plane bounce damping", &Kr, 0.01f, 1.f);
	ImGui::SliderFloat("plane friction", &Kmu, 0.01f, 1.f);

	ImGui::SliderFloat("air damping", &Ka, 0.0001f, 0.09f, "%.5f");

	ImGui::SliderFloat("cloth p mass", &cloth_p_mass, 0.001f, 0.1f);
	ImGui::SliderFloat("spring coef", &Ks, 10.f, 70.f);
	ImGui::SliderFloat("stretch", &stretch_pct, 0.1f, 1.f);
	ImGui::SliderFloat("shear", &shear_pct, 0.1f, 1.f);
	ImGui::SliderFloat("bending", &bending_pct, 0.1f, 1.f);
	ImGui::SliderFloat("spring damping coef", &Kd, 0.00f, 1.4f);

	if(ImGui::Button("step one frame")) {
		is_rendered_frame = false;
	}

	// if(ImGui::SliderInt2("nr cloth ptcls", (int*)&nr_p, 2, 200)) {
	// 	makeClothData();
	// }
	if(ImGui::SliderInt("nr cloth ptcls", (int*)&nr_p.x, 2, 200)) {
		nr_p.y = nr_p.x;
		makeClothData();
	}

	ImGui::End();
}