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
#include <imgui.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/asset_lib.h>
#include <limbrary/model_view/renderer.h>

namespace lim
{
	ArtMap::ArtMap(const std::string_view _path, GLint _tone)
		: Texture(), tone(_tone)
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
				width = w; height = h; src_nr_channels = ch;
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
		, viewport("viewport##hatching", new FramebufferMs())
	{
		glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		//glPolygonMode(GL_FRONT, GL_LINE);

		h_prog.name = "hatching prog";
		h_prog.home_dir = APP_DIR;
		h_prog.attatch("hatching.vs").attatch("hatching.fs").link();

		h_mat.prog = &h_prog;
		h_mat.set_prog = [this](const Program& prog) 
		{
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
			prog.setUniform("tam", nr_tones, tam);
		};
		
		viewport.setClearColor({0.1f, 0.1f, 0.1f, 1.0f});

		models.push_back({});
		models.back().importFromFile("assets/models/dwarf/Dwarf_2_Low.obj", true, false);
		models.back().default_material = &h_mat;
	
		models.push_back( {} );
		models.back().name = "sphere";
		models.back().root.addMeshWithMat(&AssetLib::get().sphere);
		models.back().default_material = &h_mat;
		models.back().updateUnitScaleAndPivot();

		models.push_back({});
		models.back().importFromFile("assets/models/dwarf/Dwarf_2_Low.obj", true, false);
		models.back().default_material = &h_mat;

		const float interModels = 2.f;
		const float biasModels = -interModels*(models.size()-1)*0.5f;

		for( int i = 0; i<models.size(); i++ ) {
			models[i].position ={biasModels + interModels*i, 0, 0};
			models[i].updateModelMat();
		}

		// models.push_back( new Model("ground") );
		// models.back().meshes.push_back( mesh_maked::genPlane() );
		// models.back().root.meshes.push_back(models.back().meshes.back());
		// models.back().scale = glm::vec3(20);
		// models.back().updateModelMat();

		light.setRotate(30.f, 30.f, 10.f);
		light_model.name = "light";
		light_model.my_meshes.push_back(new MeshSphere(8, 4));
		light_model.root.addMeshWithMat(light_model.my_meshes.back()); // delete sphere when delete model!
		light_model.position = light.position;
		light_model.scale = glm::vec3(0.3f);
		light_model.updateModelMat();

		const std::string basename = "assets/images/TAM/default.bmp";
		for( int tone=0; tone<nr_tones; tone++ ) {
			std::string filename = basename;
			filename.insert(filename.rfind('.'), 1, '0'+tone);
			tam.push_back(new ArtMap(filename, 0));
		}

		scene.lights.push_back(&light);
		for(const auto& md: models) {
			scene.models.push_back(&md);
		}
	}
	AppHatching::~AppHatching()
	{
		for( ArtMap* t : tam ) {
			delete t;
		}
	}
	void AppHatching::update()
	{
		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* render to fbo in viewport */

		render(viewport.getFb(), viewport.camera, scene);
	}
	void AppHatching::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport.drawImGui();

		/* controller */
		ImGui::Begin("controller##hatching");
		
		ImGui::SliderFloat("uv scale x", &uv_scale.x, 0.1f, 20.f, "%.3f");
		ImGui::SliderFloat("uv scale y", &uv_scale.y, 0.1f, 20.f, "%.3f");

		ImGui::Checkbox("is 6-way blending", &is6way);

		ImGui::Text("<light>");
		const static float litThetaSpd = 70 * 0.001;
		const static float litPhiSpd = 360 * 0.001;
		static float litTheta = 30.f;
		static float litPhi = 30.f;
		static bool isLightDraged = false;
		isLightDraged |= ImGui::DragFloat("light yaw", &litPhi, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
		isLightDraged |= ImGui::DragFloat("light pitch", &litTheta, litThetaSpd, 0, 80, "%.3f");
		if( isLightDraged ) {
			light.setRotate(litTheta, glm::fract(litPhi/360.f)*360.f);
			light_model.position = light.position;
			light_model.updateModelMat();
		}
		ImGui::Text("pos %.1f %.1f %.1f", light.position.x, light.position.y, light.position.z);

		ImGui::SliderInt("use fixed art map", &fixed_art_map_idx, -1, nr_tones-1);
		
		ImGui::End();

		/* state view */
		ImGui::Begin("state##hatching");
		ImGui::Text("fovy: %f", viewport.camera.fovy);
		ImGui::End();
	}
}

#endif
