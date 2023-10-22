#include "app_kinematics.h"
#include <imgui.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/model_view/mesh_maked.h>

namespace lim
{
	AppKinematics::AppKinematics(): AppBase(1200, 780, APP_NAME)
		, viewport("AnimTester", new FramebufferMs(8))
	{
		Mesh* ms; 
		Model* md; 
		scene.addOwnLight(new Light());

		ms = new MeshCapsule();
		md = new Model();
		md->my_meshes.push_back(ms);
		md->root.addMeshWithMat(ms);
		scene.addOwnModel(md);

		ms = new MeshPlane();
		md = new Model();
		md->my_meshes.push_back(ms);
		md->root.addMeshWithMat(ms);
		scene.addOwnModel(md);
	}
	AppKinematics::~AppKinematics()
	{

	}
	void AppKinematics::update()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		render(viewport.getFb(), viewport.camera, scene);
	}
	void AppKinematics::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport.drawImGui();
	}
}
