#include "app_shadertoy.h"
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/log.h>
#include <fstream>

#include <limbrary/using_in_cpp/glm.h>
#include <limbrary/using_in_cpp/std.h>
using namespace lim;


AppShaderToy::AppShaderToy() : AppBase(780, 780, APP_NAME, true)
	, viewport(new FramebufferNoDepth())
	, program("im_shadertoy/shaders/hextile.glsl")
{
	iChannel0.s_wrap_param = GL_REPEAT;
	iChannel0.initFromFile("assets/images/uv_grid.jpg", true);

	key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
		if( ( GLFW_MOD_CONTROL == mods ) && ( 'R' == key ) ) {
			program.reload();
		}
	};
	dnd_callbacks[this] = [this](int count, const char **paths) {
		program.reload(paths[0]);
	};
}
AppShaderToy::~AppShaderToy()
{
}

void AppShaderToy::update() 	
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewport.getFb().bind();
	program.use();

	vec3 iResolution(viewport.content_size, 0.f);
	program.setUniform("iResolution", iResolution);

	float iTime = glfwGetTime();
	program.setUniform("iTime", iTime);

	float iTimeDelta = delta_time;
	program.setUniform("iTimeDelta", iTimeDelta);

	float iFrameRate = ImGui::GetIO().Framerate;
	program.setUniform("iTimeDelta", iTimeDelta);

	int iFrame = ImGui::GetFrameCount();
	program.setUniform("iFrame", iFrame);

	// float iChannelTime[4] = {0,};
	// program.setUniform("iChannelTime", iChannelTime);

	// float iChannelResolution = ImGui::GetIO().Framerate;
	// program.setUniform("iChannelResolution", iChannelResolution);

	vec4 iMouse = {viewport.mouse_pos.x, viewport.mouse_pos.y, 0,0};
	program.setUniform("iMouse", iMouse);

	program.setTexture("iChannel0", iChannel0.tex_id);
	// program.setUniform("iChannel1", iChannel1, 0);
	// program.setUniform("iChannel2", iChannel2, 0);
	// program.setUniform("iChannel3", iChannel3, 0);

	// vec4 iDate = {0,0,0,0};
	// program.setUniform("iDate", iDate);

	// float iSampleRate = ImGui::GetIO().Framerate;
	// program.setUniform("iSampleRate", iSampleRate);

		
	asset_lib::screen_quad->bindAndDrawGL();
	viewport.getFb().unbind();
}

void AppShaderToy::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	ImGui::Begin("controller##shadertoy");
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
	
	if(ImGui::Button("reload .fs")) {
		program.reload();
	}
	ImGui::End();

	viewport.drawImGui();
}

ProgramShaderToy::ProgramShaderToy(const char* glslPath)
	: ProgramReloadable("toy"), glsl_path(glslPath)
{
	makeMergedFragmentShader();
	attatch("canvas.vs").attatch("im_shadertoy/temp/temp.fs").link();
}
void ProgramShaderToy::reload(const char* glslPath)
{
	if(glslPath != nullptr) {
		glsl_path = glslPath;
	}
	makeMergedFragmentShader();
	ProgramReloadable::reload(GL_FRAGMENT_SHADER);
}
void ProgramShaderToy::makeMergedFragmentShader()
{
	std::ifstream top("im_shadertoy/temp/top.glsl");
	std::ifstream code(glsl_path.data());
	if(!code.is_open()) {
		log::err("no path: %s", glsl_path.data());
		top.close();
	}
	std::ifstream bottom("im_shadertoy/temp/bottom.glsl");
	std::ofstream temp("im_shadertoy/temp/temp.fs");

	temp<<top.rdbuf()<<code.rdbuf()<<bottom.rdbuf();
	top.close();
	code.close();
	bottom.close();
	temp.close();
}