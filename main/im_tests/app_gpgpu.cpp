#include "app_gpgpu.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/program.h>

using namespace lim;
using namespace glm;

namespace {
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

	Program xfb_prog;
	uint xfb;
	uint xfb_out_buf;

	float out_length[10];
	vec3 out_norm[10];

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

	glGenBuffers(1, &xfb_out_buf);
	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(out_length)+sizeof(out_norm), nullptr, GL_DYNAMIC_COPY);

	xfb_prog.attatch("im_tests/shaders/xfb.vs").link();

	glGenTransformFeedbacks(1, &xfb);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_out_buf, 0, sizeof(out_length));
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 1, xfb_out_buf, sizeof(out_length), sizeof(out_norm));


	xfb_prog.use();
	glEnable(GL_RASTERIZER_DISCARD);
	{
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
		{
			glBeginTransformFeedback(GL_POINTS);
			{
				glBindVertexArray(pos_vao);
				glDrawArrays(GL_POINTS, 0, 10);
			}
			glEndTransformFeedback();
		}
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	}
	glDisable(GL_RASTERIZER_DISCARD);
	glFlush();


	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(out_length), &out_length);
	glBindBuffer(GL_ARRAY_BUFFER, xfb_out_buf);
	glGetBufferSubData(GL_ARRAY_BUFFER, sizeof(out_length), sizeof(out_norm), &out_norm);

	for(int i=0; i<sizeof(out_norm)/sizeof(out_norm[0]); i++)
		log::pure("%f %f %f\n", out_norm[i].x, out_norm[i].y, out_norm[i].z);
	for(int i=0; i<sizeof(out_length)/sizeof(out_length[0]); i++)
		log::pure("%f\n", out_length[i]);
}
AppGpgpu::~AppGpgpu()
{
	glDeleteBuffers(1, &pos_vbo);
	glDeleteTransformFeedbacks(1, &xfb);
	glDeleteBuffers(1, &xfb_out_buf);
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