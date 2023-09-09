#include "app_model_viewer.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/model_importer.h>
#include <limbrary/log.h>
#include <limbrary/model_view/viewport_with_camera.h>
#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <vector>

using namespace lim;
using namespace std;
using namespace glm;

namespace 
{
	vector<ViewportWithCamera*> asdffff;
	vector<Model*> models;
	Program* program;

	void addModelViewer(const char* path) {
		ViewportWithCamera* vp = new ViewportWithCamera(
			fmtStrToBuf("viewport%d##model_view", (int)asdffff.size()), new RboFramebuffer());
		asdffff.push_back(vp);

		Model* md = importModelFromFile(path, true);
	}
	void drawModelsToViewports(const Light& light)
	{
		for(int i=0; i<asdffff.size(); i++) {
			const Viewport& vp = *asdffff[i];
			const Camera& cam = asdffff[i]->camera;
			const Model& md = *models[i];

			vp.framebuffer->bind();
			const Program& prog = *program;
			prog.use();
			prog.setUniform("cameraPos", cam.position);
			prog.setUniform("projMat", cam.proj_mat);
			prog.setUniform("viewMat", cam.view_mat);
			prog.setUniform("modelMat", md.model_mat);

			prog.setUniform("lightDir", light.direction);
			prog.setUniform("lightColor", light.color);
			prog.setUniform("lightInt", light.intensity);
			prog.setUniform("lightPos", light.position);

	
			for( Mesh* pMesh : md.meshes)
			{
				const Mesh& mesh = *pMesh;

				glBindVertexArray(mesh.VAO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
				glDrawElements(mesh.draw_mode, static_cast<GLuint>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
			}



			vp.framebuffer->unbind();
		}
		
	}
}


namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
		program = new Program("model_view", APP_DIR);
		program->attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();
	}
	AppModelViewer::~AppModelViewer()
	{
		for(auto vp : asdffff)
			delete vp;
		asdffff.clear();
		for(auto md : models)
			delete md;
		models.clear();
		delete program;
	}
	void AppModelViewer::update() 	{
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawModelsToViewports(light);
	}
	void AppModelViewer::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		log::drawViewer("logger##model_viewer");

		ImGui::Begin("controller##model_viewer");
		ImGui::Text("hi");
		ImGui::End();

		for(ViewportWithCamera* vp : asdffff) {
			vp->drawImGui();
		}
	}
	void AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
	{
		log::pure("%d\n", key);
		log::info("%d\n", key);
		log::warn("%d\n", key);
		log::err("%d\n", key);
	}
	void AppModelViewer::cursorPosCallback(double xPos, double yPos)
	{
	}
	void AppModelViewer::dndCallback(int count, const char **paths)
	{
		for( int i=0; i<count; i++ ) {
			const char* path = paths[i];
			addModelViewer(path);
		}
	}
}