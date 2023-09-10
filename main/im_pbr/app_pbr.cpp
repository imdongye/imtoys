#include "app_pbr.h"
#include "imgui.h"
#include "stb_image.h"
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/code_mesh.h>
#include <glm/gtx/transform.hpp>

namespace lim
{
	AppPbr::AppPbr(): AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		metal_colors.push_back( {1.000, 0.782, 0.344} ); // Gold
		metal_colors.push_back( {0.664, 0.824, 0.850} ); // Zinc
		metal_colors.push_back( {0.972, 0.960, 0.915} ); // Silver
		metal_colors.push_back( {0.955, 0.638, 0.583} ); // Corper

		prog = new Program("pbr", APP_DIR);
		prog->attatch("1.1.pbr.vs").attatch("1.1.pbr.fs").link();
		
		viewport = new ViewportWithCamera("viewport##pbr", new MsFramebuffer());
		viewport->framebuffer->clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

		model = importModelFromFile("assets/models/objs/bunny.obj", true);
		model->scale = model->scale*3.f;
		model->position = glm::vec3(15, 0, 0);
		model->updateModelMat();

		sphere = code_mesh::genSphere(50, 25);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		/* initialize static shader uniforms before rendering */
		GLuint pid = prog->use();
		setUniform(pid, "ao", 1.0f);
	}
	AppPbr::~AppPbr()
	{
		delete prog;
		delete viewport;
		delete model;
	}
	void AppPbr::update()
	{               
		/* render to fbo in viewport */
		viewport->framebuffer->bind();

		GLuint pid = prog->use();
		setUniform(pid, "beckmannGamma", beckmannGamma);
		const Camera& cam = viewport->camera;
		setUniform(pid, "view", cam.view_mat);
		setUniform(pid, "camPos", cam.position);
		setUniform(pid, "projection", cam.proj_mat);

		glm::vec3 lightPos = light_position + glm::vec3(sin(glfwGetTime() * 2.0) * 5.0, 0.0, 0.0);
		if(!movedLight) lightPos = light_position;
		setUniform(pid, "lightPosition", lightPos);
		setUniform(pid, "lightColor", light_color);

		glm::mat4 modelMat;

		///* draw light */
		modelMat = glm::translate(glm::mat4(1), lightPos);
		modelMat = glm::scale(modelMat, glm::vec3(0.5f));
		setUniform(pid, "model", modelMat);
		prog->setUniform("Kd", sphere->color);
		glBindVertexArray(sphere->VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere->EBO);
		glDrawElements(sphere->draw_mode, static_cast<GLuint>(sphere->indices.size()), GL_UNSIGNED_INT, 0);


		///* draw spheres */
		glm::vec2 pivot = {(nr_cols-1)*spacing*0.5f, (nr_rows-1)*spacing*0.5f};

		for( int row = 0; row < nr_rows; ++row ) {
			setUniform(pid, "metallic", row/(float)nr_rows);
			setUniform(pid, "isGGX", (row%2==0));

			for( int col = 0; col < nr_cols; ++col ) {
				if( row<2 ) {
					// under 0.05 to error
					setUniform(pid, "roughness", glm::clamp( col/(float)nr_cols, 0.05f, 1.0f));
					setUniform(pid, "albedo", albedo);
					setUniform(pid, "metallic", 0.f);
				}
				else {
					setUniform(pid, "roughness", glm::clamp( 1/(float)nr_cols, 0.05f, 1.0f));
					setUniform(pid, "albedo", metal_colors[col]);
					setUniform(pid, "metallic", 1.f);
				}

				modelMat = glm::translate(glm::mat4(1.0f), glm::vec3( col*spacing-pivot.x, pivot.y-row*spacing, 0.0f ));
				setUniform(pid, "model", modelMat);

				glBindVertexArray(sphere->VAO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere->EBO);
				glDrawElements(sphere->draw_mode, static_cast<GLuint>(sphere->indices.size()), GL_UNSIGNED_INT, 0);
			}
		}

		setUniform(pid, "albedo", metal_colors[0]);
		setUniform(pid, "metallic", metallic);
		setUniform(pid, "roughness", roughness);

		///* draw two test shpere */
		setUniform(pid, "isGGX", true);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		setUniform(pid, "model", modelMat);
		glBindVertexArray(sphere->VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere->EBO);
		glDrawElements(sphere->draw_mode, static_cast<GLuint>(sphere->indices.size()), GL_UNSIGNED_INT, 0);

		setUniform(pid, "isGGX", false);
		modelMat = glm::translate(glm::mat4(1), glm::vec3(spacing*3, -spacing, 0));
		modelMat *= glm::scale(glm::vec3(2.f));
		setUniform(pid, "model", modelMat);
		glBindVertexArray(sphere->VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere->EBO);
		glDrawElements(sphere->draw_mode, static_cast<GLuint>(sphere->indices.size()), GL_UNSIGNED_INT, 0);

		/* draw models */
		setUniform(pid, "isGGX", true);
		setUniform(pid, "model", model->model_mat);
		for( Mesh* mesh: model->meshes ) {
			glBindVertexArray(mesh->VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
			glDrawElements(mesh->draw_mode, static_cast<GLuint>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
		}
		
		viewport->framebuffer->unbind();
		
		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void AppPbr::renderImGui()
	{
		ImGui::DockSpaceOverViewport();
		
		// draw framebuffer on viewport gui
		viewport->drawImGui();

		ImGui::Begin("state##impbr");
		ImGui::Text((viewport->dragging)?"dragging":"not dragging");
		ImGui::Checkbox("moved light:", &movedLight);
		ImGui::ColorPicker3("albedo", (float*)&albedo);
		ImGui::SliderFloat("roughness", &roughness, 0.f, 1.f);
		ImGui::SliderFloat("metallic", &metallic, 0.f, 1.f);
		ImGui::SliderFloat("beckmannGamma", &beckmannGamma, 0.7f, 2.f);
		ImGui::End();
	}
}