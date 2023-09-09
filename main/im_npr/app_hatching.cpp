//
//  for aplicate real-time hatching paper
//	2023-1-17 / im dong ye
//
//	TODO list:
//	
//

#ifndef APP_HACHING_H
#define APP_HACHING_H

#include "app_hatching.h"
#include <stb_image.h>
#include <limbrary/log.h>
#include <limbrary/model_view/model_importer.h>
#include <imgui.h>
#include <limbrary/model_view/code_mesh.h>

namespace lim
{
	ArtMap::ArtMap(const std::string_view _path, GLint _tone)
		: TexBase(), tone(_tone)
	{
		path = _path;

		internal_format = GL_SRGB8;
		src_chanel_type = GL_UNSIGNED_BYTE;
		src_bit_per_channel = 8;

		glGenTextures(1, &tex_id);
		glBindTexture(GL_TEXTURE_2D, tex_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// From: https://www.khronos.org/opengl/wiki/Common_Mistakes#Creating_a_complete_texture
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, nr_mips-1);

		void* data;
		for( int lv=0; lv<nr_mips; lv++ ) {
			int w, h, ch;
			std::string leveledPath = path;
			leveledPath.insert(path.rfind('.'), 1, '0'+nr_mips-1-lv);

			data=stbi_load(leveledPath.c_str(), &w, &h, &ch, 0);
			if( !data ) {
				log::err("texture failed to load at path: %s\n", path.c_str());
				return;
			}
			if( lv==0 ) {
				width = w; height = h; nr_channels = ch;
			}
			aspect_ratio = width/(float)height;

			switch( ch ) {
				case 1: src_format = GL_RED; break;
				case 2: src_format = GL_RG; break;
				case 3: src_format = GL_RGB; break;
				case 4: src_format = GL_RGBA; break;
				default: log::err("texter channels is over 4\n"); return;
			}
			printf("%s loaded : lv:%d, texID:%d, %dx%d, nrCh:%d\n"
					, leveledPath.c_str(), lv, tex_id, w, h, ch);
			glTexImage2D(GL_TEXTURE_2D, lv, internal_format, w, h
							, 0, src_format, src_chanel_type, data);

			stbi_image_free(data);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, nr_mips-1);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	}

	AppHatching::AppHatching(): AppBase(1200, 780, APP_NAME)
	{
		glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		//glPolygonMode(GL_FRONT, GL_LINE);
		stbi_set_flip_vertically_on_load(true);

		program = new Program("hatching prog", APP_DIR);
		program->attatch("hatching.vs").attatch("hatching.fs").link();

		viewport = new ViewportWithCamera("viewport##hatching", new MsFramebuffer());
		viewport->framebuffer->clear_color ={0.1f, 0.1f, 0.1f, 1.0f};

		models.push_back(importModelFromFile("assets/models/dwarf/Dwarf_2_Low.obj", true));
		models.push_back(new Model(code_mesh::genSphere(50, 25), "sphere"));
		models.push_back(importModelFromFile("assets/models/objs/bunny.obj", true) );

		const float interModels = 3.5f;
		//1-0
		//2-0.5
		//3-1
		//4=1.5
		//5=2
		const float biasModels = -interModels*(models.size()-1)*0.5f;

		for( int i = 0; i<models.size(); i++ ) {
			models[i]->position ={biasModels+interModels*i, 0, 0};
			models[i]->updateModelMat();
		}

		light = new Light();
		light->distance = 10.f;
		light_model = new Model(code_mesh::genSphere(8, 4), "sphere");
		light_model->position = light->position;
		light_model->scale = glm::vec3(0.3f);
		light_model->updateModelMat();



		const std::string basename = "assets/images/TAM/default.bmp";
		for( int tone=0; tone<nr_tones; tone++ ) {
			std::string filename = basename;
			filename.insert(filename.rfind('.'), 1, '0'+tone);
			tam.push_back(new ArtMap(filename, 0));
		}
	}
	AppHatching::~AppHatching()
	{
		delete program;
		delete viewport;
		for( Model* m : models ) {
			delete m;
		}
		delete light;
		delete light_model;
		for( ArtMap* t : tam ) {
			delete t;
		}
	}
	void AppHatching::update()
	{
		/* render to fbo in viewport */
		viewport->framebuffer->bind();

		Program& prog = *program;
		prog.use();
		
		const Camera& camera = viewport->camera;
		prog.setUniform("viewMat", camera.view_mat);
		prog.setUniform("projMat", camera.proj_mat);
		prog.setUniform("cameraPos", camera.position);

		
		prog.setUniform("lightDir", light->direction);
		prog.setUniform("lightColor", light->color);
		prog.setUniform("lightInt", light->intensity);
		prog.setUniform("lightPos", light->position);

		prog.setUniform("uvScale", uv_scale);
		prog.setUniform("fixedArtMapIdx", fixed_art_map_idx);
		prog.setUniform("is6way", is6way);


		/* texture binding */
		for( GLint lv=0; lv<nr_tones; lv++ ) {
			ArtMap& am = *tam[lv];

			glActiveTexture(GL_TEXTURE0 + lv);
			glBindTexture(GL_TEXTURE_2D, am.tex_id);
		}
		int tam[6] = {5,4,3,2,1,0};
		prog.setUniform("tam", tam, nr_tones);

		prog.setUniform("modelMat", light_model->model_mat);
		Mesh* mesh = light_model->meshes.back();
		prog.setUniform("Kd", mesh->color);
		glBindVertexArray(mesh->VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
		glDrawElements(mesh->draw_mode, static_cast<GLuint>(mesh->indices.size()), GL_UNSIGNED_INT, 0);

		for( Model* m : models ) {
			prog.setUniform("modelMat", m->model_mat);
			mesh = m->meshes.back();
			prog.setUniform("Kd", mesh->color);
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
	void AppHatching::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport->drawImGui();

		/* controller */
		ImGui::Begin("controller##hatching");
		
		ImGui::SliderFloat("uv scale x", &uv_scale.x, 0.1f, 20.f, "%.3f");
		ImGui::SliderFloat("uv scale y", &uv_scale.y, 0.1f, 20.f, "%.3f");

		ImGui::Checkbox("is 6-way blending", &is6way);

		ImGui::Text("<light>");
		const float yawSpd = 360 * 0.001;
		const float pitchSpd = 80 * 0.001;
		const float distSpd = 94 * 0.001;
		bool isDraging = ImGui::SliderFloat("yaw", &light->yaw, 0, 360, "%.3f");
		isDraging |= ImGui::SliderFloat("pitch", &light->pitch, 10, 90, "%.3f");
		isDraging |= ImGui::SliderFloat("distance", &light->distance, 6, 100, "%.3f");
		if( isDraging ) {
			light->updateMembers();
			light_model->position = light->position;
			light_model->updateModelMat();
		}
		ImGui::Text("pos %f %f %f", light->position.x, light->position.y, light->position.z);

		ImGui::SliderInt("use fixed art map", &fixed_art_map_idx, -1, nr_tones-1);
		
		ImGui::End();

		/* state view */
		ImGui::Begin("state##hatching");
		ImGui::Text("fovy: %f", viewport->camera.fovy);
		ImGui::End();
	}
}

#endif
