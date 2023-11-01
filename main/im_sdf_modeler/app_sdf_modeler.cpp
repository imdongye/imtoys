#include "app_sdf_modeler.h"
#include <imgui.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/asset_lib.h>

namespace lim
{
	AppSdfModeler::AppSdfModeler(): AppBase(1200, 780, APP_NAME, false)
		, viewport("AnimTester", new Framebuffer()) // 멀티셈플링 동작 안함.
	{
		viewport.use_guizmo = true;
		viewport.camera.pivot = glm::vec3(0,1,0);
		viewport.camera.position = glm::vec3(0,1,5);
		viewport.camera.updateViewMat();
		prog.name = "sdf and ray marching";
		prog.home_dir = APP_DIR;
		prog.attatch("canvas.vs").attatch("shader.fs").link();
	}
	AppSdfModeler::~AppSdfModeler()
	{
	}
	void AppSdfModeler::update()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Camera& cam = viewport.camera;
		viewport.getFb().bind();
		prog.use();
		prog.setUniform("lightPos", light.position);
		prog.setUniform("lightInt", light.intensity);
		prog.setUniform("cameraAspect", cam.aspect);
		prog.setUniform("cameraFovy", cam.fovy);
		prog.setUniform("cameraOrthWidth", 0.f);
		prog.setUniform("cameraPos", cam.position);
		prog.setUniform("cameraPivot", cam.pivot);

		AssetLib::get().screen_quad.drawGL();
		viewport.getFb().unbind();
	}
	void AppSdfModeler::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport.drawImGui();
	}
	void AppSdfModeler::keyCallback(int key, int scancode, int action, int mods) {
		if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
			prog.reload(GL_FRAGMENT_SHADER);
		}
	}
}
