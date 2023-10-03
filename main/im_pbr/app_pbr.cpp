#include "app_pbr.h"
#include "imgui.h"
#include "stb_image.h"
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/mesh_maked.h>
#include <glm/gtx/transform.hpp>

namespace lim
{
	AppPbr::AppPbr(): AppBase(1200, 780, APP_NAME)
		, viewport("viewport##pbr", new MsFramebuffer())
		, sphere(50, 25)
	{
		
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		model.importFromFile("assets/models/objs/bunny.obj", true);

		metal_colors.push_back( {1.000, 0.782, 0.344} ); // Gold
		metal_colors.push_back( {0.664, 0.824, 0.850} ); // Zinc
		metal_colors.push_back( {0.972, 0.960, 0.915} ); // Silver
		metal_colors.push_back( {0.955, 0.638, 0.583} ); // Corper

		prog.name = "pbr";
		prog.home_dir = APP_DIR;
		prog.attatch("1.1.pbr.vs").attatch("1.1.pbr.fs").link();

		viewport.getFb().clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
		viewport.camera.move_free_spd = 6.f;
		viewport.camera.shiftPos({0,0,10});

		model.scale = model.scale*3.f;
		model.position = glm::vec3(15, 0, 0);
		model.updateModelMat();

		/* initialize static shader uniforms before rendering */
		prog.use().bind("ao", 1.0f);
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

		prog.bind("beckmannGamma", beckmannGamma);
		const Camera& cam = viewport.camera;
		prog.bind("view", cam.view_mat);
		prog.bind("camPos", cam.position);
		prog.bind("projection", cam.proj_mat);

		glm::vec3 lightPos = light_position + glm::vec3(sin(glfwGetTime() * 2.0) * 5.0, 0.0, 0.0);
		if(!movedLight) lightPos = light_position;
		prog.bind("lightPosition", lightPos);
		prog.bind("lightColor", light_color);

		glm::mat4 modelMat;

		///* draw light */
		modelMat = glm::translate(glm::mat4(1), lightPos);
		modelMat = glm::scale(modelMat, glm::vec3(0.5f));
		prog.bind("model", modelMat);
		prog.bind("Kd", glm::vec3(1,1,1));
		sphere.drawGL();


		///* draw spheres */
		glm::vec2 pivot = {(nr_cols-1)*spacing*0.5f, (nr_rows-1)*spacing*0.5f};

		for( int row = 0; row < nr_rows; ++row ) {
			prog.bind("metallic", row/(float)nr_rows);
			prog.bind("isGGX", (row%2==0));

			for( int col = 0; col < nr_cols; ++col ) {
				if( row<2 ) {
					// under 0.05 to error
					prog.bind("roughness", glm::clamp( col/(float)nr_cols, 0.05f, 1.0f));
					prog.bind("albedo", albedo);
					prog.bind("metallic", 0.f);
				}
				else {
					prog.bind("roughness", glm::clamp( 1/(float)nr_cols, 0.05f, 1.0f));
					prog.bind("albedo", metal_colors[col]);
					prog.bind("metallic", 1.f);
				}

				modelMat = glm::translate(glm::mat4(1.0f), glm::vec3( col*spacing-pivot.x, pivot.y-row*spacing, 0.0f ));
				prog.bind("model", modelMat);

				sphere.drawGL();
			}
		}

		prog.bind("albedo", metal_colors[0]);
		prog.bind("metallic", metallic);
		prog.bind("roughness", roughness);

		///* draw two test shpere */
		prog.bind("isGGX", true);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		prog.bind("model", modelMat);
		sphere.drawGL();


		prog.bind("isGGX", false);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, -spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		prog.bind("model", modelMat);
		sphere.drawGL();


		/* draw models */
		prog.bind("isGGX", true);
		prog.bind("model", model.model_mat);
		for( const Mesh* mesh: model.my_meshes ) {
			mesh->drawGL();
		}
		
		viewport.getFb().unbind();
	}
	void AppPbr::renderImGui()
	{
		ImGui::DockSpaceOverViewport();
		
		// draw getFb() on viewport gui
		viewport.drawImGui();

		ImGui::Begin("state##impbr");
		ImGui::Text((viewport.dragging)?"dragging":"not dragging");
		ImGui::Checkbox("moved light:", &movedLight);
		ImGui::ColorPicker3("albedo", (float*)&albedo);
		ImGui::SliderFloat("roughness", &roughness, 0.f, 1.f);
		ImGui::SliderFloat("metallic", &metallic, 0.f, 1.f);
		ImGui::SliderFloat("beckmannGamma", &beckmannGamma, 0.7f, 2.f);
		ImGui::End();
	}
}