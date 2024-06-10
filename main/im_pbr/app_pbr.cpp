#include "app_pbr.h"
#include "imgui.h"
#include "stb_image.h"
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/mesh_maked.h>
#include <glm/gtx/transform.hpp>

namespace lim
{
	AppPbr::AppPbr(): AppBase(1200, 780, APP_NAME)
		, viewport("viewport##pbr", new FramebufferMs())
		, sphere(50, 25)
	{
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


		metal_colors.push_back( {1.000, 0.782, 0.344} ); // Gold
		metal_colors.push_back( {0.664, 0.824, 0.850} ); // Zinc
		metal_colors.push_back( {0.972, 0.960, 0.915} ); // Silver
		metal_colors.push_back( {0.955, 0.638, 0.583} ); // Corper

		prog.name = "pbr";
		prog.home_dir = APP_DIR;
		prog.attatch("1.1.pbr.vs").attatch("1.1.pbr.fs").link();

		viewport.setClearColor({0.1f, 0.1f, 0.1f, 1.0f});
		viewport.camera.spd_free_move = 6.f;
		viewport.camera.moveShift({0,0,10});
		viewport.camera.updateViewMat();

		model.importFromFile("assets/models/objs/bunny.obj");
		model.setUnitScaleAndPivot();
		model.tf->scale = glm::vec3(3.f);
		model.tf->pos = glm::vec3(15, 0, 0);
		model.tf->update();

		/* initialize static shader uniforms before rendering */
		prog.use().setUniform("ao", 1.0f);
	}
	AppPbr::~AppPbr()
	{
	}
	void AppPbr::update()
	{
		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		/* render to fbo in viewport */
		viewport.getFb().bind();

		prog.use();

		prog.setUniform("beckmannGamma", beckmannGamma);
		const Camera& cam = viewport.camera;
		prog.setUniform("view", cam.mtx_View);
		prog.setUniform("camPos", cam.position);
		prog.setUniform("projection", cam.mtx_Proj);

		glm::vec3 light_Pos = light_position + glm::vec3(sin(glfwGetTime() * 2.0) * 5.0, 0.0, 0.0);
		if(!movedLight) light_Pos = light_position;
		prog.setUniform("light_Position", light_Pos);
		prog.setUniform("light_Color", light_color);

		glm::mat4 modelMat;

		///* draw light */
		modelMat = glm::translate(glm::mat4(1), light_Pos);
		modelMat = glm::scale(modelMat, glm::vec3(0.5f));
		prog.setUniform("model", modelMat);
		prog.setUniform("Kd", glm::vec3(1,1,1));
		sphere.bindAndDrawGL();


		///* draw spheres */
		glm::vec2 pivot = {(nr_cols-1)*spacing*0.5f, (nr_rows-1)*spacing*0.5f};

		for( int row = 0; row < nr_rows; ++row ) {
			prog.setUniform("metallic", row/(float)nr_rows);
			prog.setUniform("isGGX", (row%2==0));

			for( int col = 0; col < nr_cols; ++col ) {
				if( row<2 ) {
					// under 0.05 to error
					prog.setUniform("roughness", glm::clamp( col/(float)nr_cols, 0.05f, 1.0f));
					prog.setUniform("albedo", albedo);
					prog.setUniform("metallic", 0.f);
				}
				else {
					prog.setUniform("roughness", glm::clamp( 1/(float)nr_cols, 0.05f, 1.0f));
					prog.setUniform("albedo", metal_colors[col]);
					prog.setUniform("metallic", 1.f);
				}

				modelMat = glm::translate(glm::mat4(1.0f), glm::vec3( col*spacing-pivot.x, pivot.y-row*spacing, 0.0f ));
				prog.setUniform("model", modelMat);

				sphere.bindAndDrawGL();
			}
		}

		prog.setUniform("albedo", metal_colors[0]);
		prog.setUniform("metallic", metallic);
		prog.setUniform("roughness", roughness);

		///* draw two test shpere */
		prog.setUniform("isGGX", true);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		prog.setUniform("model", modelMat);
		sphere.bindAndDrawGL();


		prog.setUniform("isGGX", false);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, -spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		prog.setUniform("model", modelMat);
		sphere.bindAndDrawGL();


		/* draw models */
		prog.setUniform("isGGX", true);
		prog.setUniform("model", model.tf->mtx);
		for( const Mesh* mesh: model.own_meshes ) {
			mesh->bindAndDrawGL();
		}
		
		viewport.getFb().unbind();
	}
	void AppPbr::updateImGui()
	{
		ImGui::DockSpaceOverViewport();
		
		// draw getFb() on viewport gui
		viewport.drawImGui();

		ImGui::Begin("state##impbr");
		ImGui::Text((viewport.is_dragged)?"is_dragged":"not is_dragged");
		ImGui::Checkbox("moved light:", &movedLight);
		ImGui::ColorPicker3("albedo", (float*)&albedo);
		ImGui::SliderFloat("roughness", &roughness, 0.f, 1.f);
		ImGui::SliderFloat("metallic", &metallic, 0.f, 1.f);
		ImGui::SliderFloat("beckmannGamma", &beckmannGamma, 0.7f, 2.f);
		ImGui::End();
	}
}