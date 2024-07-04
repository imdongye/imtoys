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
		LightDirectional* lit = new LightDirectional();
		scene.addOwn(lit);
		
		Model* md = new Model();
		scene.addOwn(md);

		Material* mat = new Material;
		md->addOwn(mat);

		Mesh* capsule = new MeshCapsule();
		md->addOwn(capsule);
		RdNode* capsuleNode = md->root.makeChild();
		capsuleNode->addMsMat(capsule, mat);
		capsuleNode->tf.pos = {0,1,0};
		capsuleNode->tf.update();

		Mesh* plane = new MeshPlane();
		md->addOwn(plane);
		RdNode* planeNode = md->root.makeChild();
		planeNode->addMsMat(plane, mat);
		planeNode->tf.pos = {0,-1,0};
		planeNode->tf.update();
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
	void AppKinematics::updateImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport.drawImGui();

		ImGui::Begin("camera");
		ImGui::Text("%f %f %f", viewport.camera.pos.x, viewport.camera.pos.y,viewport.camera.pos.z);
		ImGui::Text("%f %f %f", viewport.camera.pivot.x, viewport.camera.pivot.y,viewport.camera.pivot.z);
		const Transform& capsuleTf = *scene.own_mds[0]->tf;
		ImGui::Text("model");
		ImGui::Text("%f %f %f", capsuleTf.pos.x, capsuleTf.pos.y, capsuleTf.pos.z);
		for(int i=0;i<4;i++) for(int j=0;j<4;j++) {
			ImGui::Text("%.1f", capsuleTf.mtx[j][i]);
			if(j<3) ImGui::SameLine();
		}
		ImGui::End();
	}
}
