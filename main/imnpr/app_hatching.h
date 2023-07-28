//
//  for aplicate real-time hatching paper
//	2023-1-17 / im dong ye
//
//	TODO list:
//	
//

#ifndef APP_HACHING_H
#define APP_HACHING_H

#include "art_map.h"

namespace lim
{
	class AppHatching: public AppBase
	{
	public:
		inline static constexpr const char *APP_DIR =  "imnpr/";
		inline static constexpr const char *APP_NAME = "real-time hatching";
		inline static constexpr const char *APP_DISC = "aplicate real-time hatching paper";
	private:
		bool start_dragging = false;
		AutoCamera *camera;
		Program *program;
		Viewport *viewport;
		std::vector<Model*> models;
		Light *light;
		Model *light_model;
	private:
		static constexpr GLint nr_tones = 6;
		glm::vec2 uv_scale = {3.5f, 3.5f};
		int fixed_art_map_idx=-1;
		bool is6way = true;
		std::vector<ArtMap*> tam; // Tonal Art Map

	public:
		AppHatching(): AppBase(1080, 1080, APP_NAME)
		{
			glEnable(GL_CULL_FACE);
			//glCullFace(GL_FRONT);
			//glPolygonMode(GL_FRONT, GL_LINE);
			stbi_set_flip_vertically_on_load(true);

			program = new Program("hatching prog", APP_DIR);
			program->attatch("hatching.vs").attatch("hatching.fs").link();

			viewport = new Viewport(new MsFramebuffer());
			viewport->framebuffer->clear_color ={0.1f, 0.1f, 0.1f, 1.0f};

			camera = new AutoCamera(window, viewport, AutoCamera::VM_FREE);

			models.push_back(ModelLoader::loadFile("common/archive/dwarf/Dwarf_2_Low.obj", true));
			models.push_back(new Model(MeshGenerator::genSphere(50, 25), "sphere"));
			models.push_back( ModelLoader::loadFile("common/archive/meshes/stanford-bunny.obj", true) );

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
			light_model = new Model(MeshGenerator::genSphere(8, 4), "sphere");
			light_model->position = light->position;
			light_model->scale = glm::vec3(0.3f);
			light_model->updateModelMat();



			const std::string basename = "imnpr/TAM/default.bmp";
			for( int tone=0; tone<nr_tones; tone++ ) {
				std::string filename = basename;
				filename.insert(filename.rfind('.'), 1, '0'+tone);
				tam.push_back(new ArtMap(filename, 0));
			}
		}
		~AppHatching()
		{
			delete camera;
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
	private:
		virtual void update() final
		{
			/* render to fbo in viewport */
			viewport->framebuffer->bind();

			Program& prog = *program;
			prog.use();

			prog.setUniform("viewMat", camera->view_mat);
			prog.setUniform("projMat", camera->proj_mat);
			prog.setUniform("cameraPos", camera->position);

			light->setUniforms(prog);

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
			light_model->meshes.back()->draw();

			for( Model* m : models ) {
				prog.setUniform("modelMat", m->model_mat);
				m->meshes.back()->draw();
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
		virtual void renderImGui() final
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
			ImGui::Text("fovy: %f", camera->fovy);
			ImGui::End();
		}
	};
}

#endif
