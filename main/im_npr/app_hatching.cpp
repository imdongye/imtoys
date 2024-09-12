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
#include <imgui.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/model_view/scene.h>


lim::ArtMap::ArtMap(const char* _path, GLint _tone)
	: Texture(), tone(_tone)
{
	path = _path;

	internal_format = GL_SRGB8;
	src_chanel_type = GL_UNSIGNED_BYTE;
	bit_per_channel = 8;

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




lim::AppHatching::AppHatching(): AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferMs(8))
{
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glPolygonMode(GL_FRONT, GL_LINE);

	h_prog.name = "hatching prog";
	h_prog.home_dir = APP_DIR;
	h_prog.attatch("hatching.vs").attatch("hatching.fs").link();
	auto setProg = [this](const Program& prog) 
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
	h_mat.prog = &h_prog;
	h_mat.set_prog = setProg;
	
	viewport.setClearColor({0.1f, 0.1f, 0.1f, 1.0f});

	Model* md = new Model();
	md->importFromFile("assets/models/dwarf/Dwarf_2_Low.obj", false, true);
	md->setSameMat(&h_mat);
	scene.addOwn(md);
	
	md = new Model("sphere");
	md->root.ms = &AssetLib::get().sphere;
	md->root.mat = &h_mat;
	scene.addOwn(md);

	md = new Model();
	md->importFromFile("assets/models/dwarf/Dwarf_2_Low.obj", false, true);
	md->setSameMat(&h_mat);
	scene.addOwn(md);

	const float interModels = 2.f;
	const float biasModels = -interModels*(scene.own_mds.size()-1)*0.5f;

	for( int i = 0; i<scene.own_mds.size(); i++ ) {
		scene.own_mds[i]->root.tf.pos ={biasModels + interModels*i, 0, 0};
		scene.own_mds[i]->root.tf.update();
		scene.own_mds[i]->root.updateGlobalTransform();
	}


	/* ground */
	md = new Model("ground");
	md->addOwn(new MeshPlane());
	md->own_meshes.back()->initGL();
	md->root.ms = md->own_meshes.back().raw;
	md->root.mat = &h_mat;
	md->root.tf.scale = glm::vec3(20);
	md->root.tf.update();
	md->root.updateGlobalTransform();
	scene.addOwn(md);



	const std::string basename = "assets/images/TAM/default.bmp";
	for( int tone=0; tone<nr_tones; tone++ ) {
		std::string filename = basename;
		filename.insert(filename.rfind('.'), 1, '0'+(char)tone);
		tam.push_back(new ArtMap(filename.c_str(), 0));
	}

	scene.addOwn(new LightDirectional());
}
lim::AppHatching::~AppHatching()
{
	for( ArtMap* t : tam ) {
		delete t;
	}
}
void lim::AppHatching::update()
{
	// clear backbuffer
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* render to fbo in viewport */

	scene.render(viewport.getFb(), viewport.camera, true);
}
void lim::AppHatching::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	LimGui::Viewport(viewport);

	/* controller */
	ImGui::Begin("controller##hatching");
	
	ImGui::SliderFloat("uv scale x", &uv_scale.x, 0.1f, 20.f, "%.3f");
	ImGui::SliderFloat("uv scale y", &uv_scale.y, 0.1f, 20.f, "%.3f");

	ImGui::Checkbox("is 6-way blending", &is6way);

	ImGui::Text("<light>");
	const static float litThetaSpd = 70 * 0.001;
	const static float litPhiSpd = 360 * 0.001;
	static bool isLightDraged = false;
	LightDirectional& light = *scene.own_dir_lits[0];
	isLightDraged |= ImGui::DragFloat("light theta", &light.tf.theta, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
	isLightDraged |= ImGui::DragFloat("light phi",   &light.tf.phi, litThetaSpd, 0, 80, "%.3f");
	if( isLightDraged ) {
		light.tf.updateWithRotAndDist();
	}
	ImGui::Text("pos %.1f %.1f %.1f", light.tf.pos.x, light.tf.pos.y, light.tf.pos.z);

	ImGui::SliderInt("use fixed art map", &fixed_art_map_idx, -1, nr_tones-1);
	
	ImGui::End();

	/* state view */
	ImGui::Begin("state##hatching");
	ImGui::Text("fovy: %f", viewport.camera.fovy);
	ImGui::End();
}

#endif
