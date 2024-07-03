#include "app_gpgpu.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/program.h>

using namespace lim;
using namespace glm;

namespace {
	uint xfb;
	uint pos_vbo, pos_vao;
	vec3 pos_data[10] = {
		{0, 1, 0},
		{0, 1, 1},
		{1, 1, 0},
		{1, 1, 1},
		{0, 0, 1},
		{1, 1, 0},
		{1, 1, 0},
		{0, 1, 2},
		{0, 1, 0},
		{0, 1, 3},
	};
	uint out_buf_id;
	vec3 out_norm[10];
	float out_length[10];

	Program xfb_prog;
}

AppGpgpu::AppGpgpu() : AppBase(1200, 780, APP_NAME)
{
	glGenVertexArrays(1, &pos_vao);
	glBindVertexArray(pos_vao);

	glGenBuffers(1, &pos_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	xfb_prog.attatch("im_tests/shaders/xfb.vs").link();

	glCreateTransformFeedbacks(1, &xfb);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);


	glGenBuffers(1, &out_buf_id);
	glBindBuffer(GL_ARRAY_BUFFER, out_buf_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(out_norm)+sizeof(out_length), nullptr, GL_DYNAMIC_COPY);
	glTransformFeedbackBufferRange(xfb, 0, out_buf_id, 0, sizeof(out_norm));
	glTransformFeedbackBufferRange(xfb, 1, out_buf_id, sizeof(out_norm), sizeof(out_length));
	
	const char* vars[] = {"out_norm", "out_length"};
	uint pid = xfb_prog.getPid();
	glTransformFeedbackVaryings(pid, 2, vars, GL_SEPARATE_ATTRIBS);
	glLinkProgram(pid);



	glUseProgram(pid);
	glEnable(GL_RASTERIZER_DISCARD);
	glBindVertexArray(pos_vao);
	glBeginTransformFeedback(GL_POINTS);
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);

	glBindBuffer(GL_ARRAY_BUFFER, out_buf_id);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(out_norm), &out_norm);
	glDrawArrays(GL_POINT, 0, 10);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(out_length), &out_length);

	for(int i=0; i<sizeof(out_norm)/sizeof(out_norm[0]); i++)
		log::pure("%f %f %f\n", out_norm[i]);
	for(int i=0; i<sizeof(out_length)/sizeof(out_length[0]); i++)
		log::pure("%f\n", out_length[i]);

	/*glCreateBuffers(1, &x_buffer);
	glNamedBufferStorage(x_buffer, buf_size, nullptr, 0);*/
	//glBindBuffer(GL_TRANSFORM_FEEDBACK, x_buffer);
	//glBufferData(x_buffer, 1024*1024, nullptr, GL_STATIC_DRAW)
	/*glTransformFeedbackBufferRange(xfb, 0, x_buffer, 0, buf_size/2);
	glTransformFeedbackBufferRange(xfb, 1, x_buffer, buf_size / 2, buf_size / 2);*/
	/*glCreateTransformFeedbacks(n, ids);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, id);
	glIsTransformFeedback(id);
	glDeleteTransformFeedbacks(n, ids);
	glTransformFeedbackBufferBase(xfb, index, buffer);*/


}
AppGpgpu::~AppGpgpu()
{
	glDeleteBuffers(1, &pos_vbo);
	glDeleteTransformFeedbacks(1, &xfb);
	glDeleteBuffers(1, &out_buf_id);
}
void AppGpgpu::update() 
{
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void AppGpgpu::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");
	

	ImGui::Begin("test window##template");
	ImGui::Text("win size : %d %d", win_width, win_height);
	ImGui::Text("fb size  : %d %d", fb_width, fb_height);
	ImGui::End();
}