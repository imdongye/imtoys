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
		scene.addOwnLight(new Light());
		
		scene.addOwnModel(new Model());
		scene.my_mds.back()->my_meshes.push_back(new MeshCapsule());
		scene.my_mds.back()->root.addMeshWithMat(
			scene.my_mds.back()->my_meshes.back()
		);
		scene.addOwnModel(new Model());
		scene.my_mds.back()->my_meshes.push_back(new MeshPlane());
		scene.my_mds.back()->root.addMeshWithMat(
			scene.my_mds.back()->my_meshes.back()
		);
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

		ImGui::Begin("camera");
		ImGui::Text("%f %f %f", viewport.camera.position.x, viewport.camera.position.y,viewport.camera.position.z);
		ImGui::Text("%f %f %f", viewport.camera.pivot.x, viewport.camera.pivot.y,viewport.camera.pivot.z);
		const Model* md = scene.models[0];
		ImGui::Text("model");
		ImGui::Text("%f %f %f", md->position.x, md->position.y, md->position.z);
		for(int i=0;i<4;i++) for(int j=0;j<4;j++) {
			ImGui::Text("%.1f", md->transform[j][i]);
			if(j<3) ImGui::SameLine();
		}
		ImGui::End();
	}
}
