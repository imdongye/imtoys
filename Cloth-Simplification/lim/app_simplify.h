//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. 헤더파일include 중복관리
//	3. dnd 여러개들어왔을때 모델파일만 골라서 입력받게 조건처리
//	4. figure out [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
//	5. https://github.com/AndrewBelt/osdialog 로 multi platform open file dialog
//	6. baker.h로 분리
//

#ifndef SIMPLYFY_APP_H
#define SIMPLYFY_APP_H


#if defined(_MSC_VER) && !defined(STBI_MSC_SECURE_CRT)
#define STBI_MSC_SECURE_CRT
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include<stb/stb_image_write.h>

namespace lim
{
	class SimplifyApp: public AppBase
	{
	private:
		float cameraMoveSpeed;
		Light light;

		const char* exportPath = "result/";
		int selectedBakeSizeIdx;
		std::vector<const char*> bakeSizeList;
		Framebuffer bakedNormalMap;
		Program bakerProg;

		Camera* cameras[2];
		Model* models[2];
		Scene* scenes[2];
		Viewport* viewports[2];
		std::vector<Program*> programs;
	public:
		SimplifyApp(): AppBase(1200, 960), cameraMoveSpeed(1.6f), light()
			, bakeSizeList({"256", "512", "1024", "2048", "4096", "8192"}), selectedBakeSizeIdx(2)
			, bakedNormalMap(), bakerProg("Normal Map Baker")
		{
			bakedNormalMap.clearColor = glm::vec4(0.5, 0.5, 1, 1);
			bakedNormalMap.resize(atoi(bakeSizeList[selectedBakeSizeIdx]));
			bakerProg.attatch("uv_view.vs").attatch("uv_view.fs").link();


			programs.push_back(new Program("Normal Dot View"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("ndv.fs").link();

			programs.push_back(new Program("Normal Dot Light"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("ndl.fs").link();

			programs.push_back(new Program("Diffuse"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("diffuse.fs").link();

			programs.push_back(new Program("Uv"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("uv.fs").link();

			programs.push_back(new Program("Bump"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("bump.fs").link();

			programs.push_back(new Program("Shadowed"));
			programs.back()->attatch("shadowed.vs").attatch("shadowed.fs").link();

			programs.push_back(new Program("Uv View"));
			programs.back()->attatch("uv_view.vs").attatch("uv_view.fs").link();

			models[0] = new Model("archive/dwarf/Dwarf_2_Low.obj", programs[0], true);
			models[1] = nullptr;

			scenes[0] = new Scene(programs[0], light);
			scenes[0]->setModel(models[0]);

			scenes[1] = new Scene(programs[0], light);
			scenes[1]->setModel(models[0]);

			cameras[0] = new Camera(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0, 0, -1), scr_width/(float)scr_height);
			cameras[1] = new Camera(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0, 0, -1), scr_width/(float)scr_height);

			Viewport* originalView = new Viewport(cameras[0]);
			viewports[0] = originalView;

			Viewport* simplifiedView = new Viewport(cameras[1]);
			viewports[1] = simplifiedView;

			init_callback();
			imgui_modules::initImGui(window);
		}
		~SimplifyApp()
		{
			for( Camera* camera : cameras ) delete camera;
			for( Model* model : models ) delete model;
			for( Scene* scene : scenes ) delete scene;
			for( Viewport* viewport : viewports ) delete viewport;
			for( Program* program : programs ) delete program;

			imgui_modules::destroyImGui();
		}
	private:
		void loadModel(std::string_view path)
		{
			double start = glfwGetTime();
			Logger::get().log("Loading %s..... . ... ..... .. .... .. . . .. .\n", path.data());
			Program* usedProg = models[0]->program;
			delete models[0]; models[0]=nullptr;
			models[0] = new Model(path.data(), usedProg, true);
			scenes[0]->setModel(models[0]);
			scenes[1]->setModel(models[0]);
			AppPref::get().pushPathWithoutDup(path.data());

			double loadTime = glfwGetTime() - start;
			Logger::get().log("Done! in %.3f sec.  \n", loadTime);
		}
		void simplifyModel(float lived_pct = 0.8f
						   , int version = 0, int agressiveness=7, bool verbose=true)
		{
			double start = glfwGetTime();
			Logger::get().log("\nSimplifing %s..... . ... ... .. .. . .  .\n", models[0]->name.c_str());

			if( models[1] != nullptr ) delete models[1];

			models[1] = fqms::simplifyModel(scenes[0]->model, lived_pct, version, agressiveness, verbose);
			scenes[1]->setModel(models[1]);

			double simpTime = glfwGetTime() - start;
			Logger::get().simpTime = simpTime;
			Logger::get().log("Done! %d => %d in %.3f sec.  \n"
							  , models[0]->verticesNum, models[1]->verticesNum, simpTime);
		}
		// 0:obj, 1:fbx, ...
		void exportModel(int format)
		{
			if( models[1] == nullptr ) {
				Logger::get()<<"need simplification"<<Logger::endl;
				return;
			}
			double start = glfwGetTime();
			std::string fullPath(exportPath);
			fullPath += models[1]->name+".png";
			Logger::get().log("Exporting %s.. .. ...... ...  .... .. . .... . .\n", fullPath.c_str());
			switch( format ) {
			case 0:
				models[1]->exportObj(exportPath);
				break;
			}

			Logger::get().log("Done! in %.3f sec.  \n", glfwGetTime()-start);
		}
		// From: https://stackoverflow.com/questions/62007672/png-saved-from-opengl-framebuffer-using-stbi-write-png-is-shifted-to-the-right
		void bakeNormalMap()
		{
			GLuint pid = bakerProg.use();
			GLuint w = bakedNormalMap.width;
			GLuint h = bakedNormalMap.height;
			static GLubyte* data = new GLubyte[3*w*h];
			std::string fullPath(exportPath);
			fullPath += models[0]->name+".png";

			bakedNormalMap.bind();
			for( Mesh* mesh : models[0]->meshes )
				mesh->draw(pid);
			bakedNormalMap.unbind();

			glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			Logger::get()<<"done"<<fullPath<<stbi_write_png(fullPath.c_str(), w, h, 3, data, w*3)<<Logger::endll;
		}
		virtual void update() final
		{
			processInput();

			//scenes[0]->render(0, scr_width, scr_height, cameras[0]); 
			scenes[0]->render(viewports[0]);
			scenes[1]->render(viewports[1]);

			renderImGui();

			/* render to screen */
			//scenes[0]->render(0, scr_width, scr_height, camera[0]);
		}
		void renderImGui()
		{
			imgui_modules::beginImGui();

			imgui_modules::ShowExampleAppDockSpace([&]() {
				if( ImGui::BeginMenu("File") ) {
					if( ImGui::BeginMenu("Open Recent") ) {
						typename std::vector<std::string>::reverse_iterator iter;
						for( iter = AppPref::get().recentModelPaths.rbegin();
							iter != AppPref::get().recentModelPaths.rend(); iter++ ) {
							if( ImGui::MenuItem((*iter).c_str()) ) {
								loadModel((*iter));
								break;
							}
						}
						ImGui::EndMenu();
					}
					if( ImGui::BeginMenu("Export") ) {
						if( ImGui::MenuItem(".obj") ) {
							exportModel(0);
						}
						if( ImGui::MenuItem(".fbx") ) {
							exportModel(1);
						}
						ImGui::EndMenu();
					}
					if( ImGui::Button("Clear Recent") ) {
						AppPref::get().clearData();
					}
					ImGui::EndMenu();
				}
			});

			ImGui::ShowDemoWindow();

			/* 실행후 겹쳤을때 그리는순서에 따라서 위로오는게 결정됨 */
			// 중요한게 뒤로 오게 해야함
			viewports[1]->drawImGui();
			viewports[0]->drawImGui();

			Logger::get().drawImGui();

			if( ImGui::Begin("Simplify Options") ) {
				// simplification
				static float pct = 0.8f;
				ImGui::SliderFloat("percent", &pct, 0.0f, 1.0f);
				static int selectedVersion = 1;
				const char* versionComboList[3] ={"agressiveness", "max_considered", "lossless"};
				ImGui::Combo("version", &selectedVersion, versionComboList, 3);
				static int agressiveness = 7;
				ImGui::SliderInt("agressiveness", &agressiveness, 5, 8);
				static bool verbose = true;
				ImGui::Checkbox("verbose", &verbose);
				if( ImGui::Button("Simplify") ) {
					simplifyModel(pct, selectedVersion, agressiveness, verbose);
				}

				ImGui::SameLine();
				ImGui::Text("target triangles = %d", static_cast<int>(scenes[0]->model->trianglesNum*pct));
				ImGui::Text("simplified in %lf sec", Logger::get().simpTime);
				ImGui::NewLine();
				ImGui::Separator();

				// baking
				if( ImGui::Combo("texture resolution", &selectedBakeSizeIdx, bakeSizeList.data(), bakeSizeList.size()) ) {
					bakedNormalMap.resize(atoi(bakeSizeList[selectedBakeSizeIdx]));
				}
				if( ImGui::Button("Bake normal map") ) {
					bakeNormalMap();
				}

			} ImGui::End();


			if( ImGui::Begin("Viewing Options") ) {
				static bool isSameCamera = false;
				if( ImGui::Checkbox("use same camera", &isSameCamera) ) {
					if( isSameCamera )
						viewports[1]->camera = cameras[0];
					else
						viewports[1]->camera = cameras[1];
				}

				ImGui::SliderFloat("move speed", &cameraMoveSpeed, 1.0f, 3.0f);

				static int prog_idx = 0;
				std::vector<const char*> comboList;
				for( Program* prog : programs )
					comboList.push_back(prog->name.c_str());
				if( ImGui::Combo("shader", &prog_idx, comboList.data(), comboList.size()) ) {
					for( Scene* scene : scenes ) {
						scene->model->program = programs[prog_idx];
						scene->ground->program = programs[prog_idx];
					}
				}
				const float yawSpd = 360*0.001;
				if( ImGui::DragFloat("light yaw", &light.yaw, yawSpd, -10, 370, "%.3f") )
					light.updateMembers();
				const float pitchSpd = 70*0.001;
				if( ImGui::DragFloat("light pitch", &light.pitch, pitchSpd, -100, 100, "%.3f") )
					light.updateMembers();

				ImGui::Text("pos %f %f %F", light.position.x, light.position.y, light.position.z);
			} ImGui::End();

			if( ImGui::Begin("Model Status")&& scenes[0]->model!=nullptr ) {
				ImGui::Text("<Original model>");
				lim::Model& ori = *(scenes[0]->model);
				ImGui::Text("name : %s", ori.name.c_str());
				ImGui::Text("#verteces : %u", ori.verticesNum);
				ImGui::Text("#triangles : %u", ori.trianglesNum);
				ImGui::Text("#meshes : %lu", ori.meshes.size());
				ImGui::Text("#textures : %lu", ori.textures_loaded.size());
				lim::Model& simp = *(scenes[1]->model);
				ImGui::NewLine();
				ImGui::Text("<Simplified model>");
				ImGui::Text("#verteces : %u", simp.verticesNum);
				ImGui::Text("#triangles : %u", simp.trianglesNum);
			} ImGui::End();

			/* show texture */
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});


			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
				data->DesiredSize.x = LIM_MAX(data->DesiredSize.x, data->DesiredSize.y);
				data->DesiredSize.y = data->DesiredSize.x;
			});
			if( ImGui::Begin("Baked Normal Map") ) {
				ImVec2 vMin = ImGui::GetWindowContentRegionMin();
				ImVec2 vMax = ImGui::GetWindowContentRegionMax();
				glm::vec2 rectSize{vMax.x-vMin.x, vMax.y-vMin.y};
				//ImGui::Text("%f %f", rectSize.x, rectSize.y);
				const float minLength = LIM_MIN(rectSize.x, rectSize.y);
				ImGui::Image(reinterpret_cast<void*>(bakedNormalMap.getRenderedTex()), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
			}ImGui::End();

			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
				data->DesiredSize.x = LIM_MAX(data->DesiredSize.x, data->DesiredSize.y);
				data->DesiredSize.y = data->DesiredSize.x;
			});
			if( ImGui::Begin("shadowMap") && light.shadowEnabled ) {
				ImVec2 vMin = ImGui::GetWindowContentRegionMin();
				ImVec2 vMax = ImGui::GetWindowContentRegionMax();
				glm::vec2 rectSize{vMax.x-vMin.x, vMax.y-vMin.y};
				//ImGui::Text("%f %f", rectSize.x, rectSize.y);
				const float minLength = LIM_MIN(rectSize.x, rectSize.y);
				ImGui::Image(reinterpret_cast<void*>(light.shadowMap.getRenderedTex()), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
			}ImGui::End();
			ImGui::PopStyleVar();

			imgui_modules::endImGui(scr_width, scr_height);
		}
		void init_callback()
		{
			//wData.drop_callback = std::bind(&SimplifyApp::drop_callback, this, std::placeholders::_1, std::placeholders::_2);
			wData.win_size_callback = [this](int width, int height) {
				return this->win_size_callback(width, height); };

			wData.key_callback = [this](int key, int scancode, int action, int mods) {
				return this->key_callback(key, scancode, action, mods); };

			wData.mouse_btn_callback = [this](int button, int action, int mods) {
				return this->mouse_btn_callback(button, action, mods); };

			wData.scroll_callback = [this](double xOffset, double yOffset) {
				return this->scroll_callback(xOffset, yOffset); };

			wData.cursor_pos_callback = [this](double xPos, double yPos) {
				return this->cursor_pos_callback(xPos, yPos); };

			wData.drop_callback = [this](int count, const char** path) {
				return this->drop_callback(count, path); };
		}
		void processInput()
		{
			for( Viewport* vp : viewports ) {
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS ) {
					float multiple = 1.0f;
					if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS )
						multiple = 1.3f;
					if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::FORWARD, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::BACKWARD, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::LEFT, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::RIGHT, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::UP, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::DOWN, deltaTime, cameraMoveSpeed*multiple);
				}
			}
		}
		void key_callback(int key, int scancode, int action, int mode)
		{
			for( Viewport* vp : viewports ) {
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
				   && key == GLFW_KEY_Z ) {
					if( action == GLFW_PRESS ) {
						camera.readyPivot();
						camera.updatePivotViewMat();
					}
					else if( action == GLFW_RELEASE ) {
						camera.readyFree();
						camera.updateFreeViewMat();
					}
				}
			}
			if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose(window, GL_TRUE);
		}
		void cursor_pos_callback(double xPos, double yPos)
		{
			for( Viewport* vp : viewports ) {
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				const float w = scr_width;
				const float h = scr_height;

				float xoff = xPos - vp->cursor_pos_x;
				float yoff = vp->cursor_pos_y - yPos; // inverse

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ) {
					if( glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS ) {
						if( glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS ) {
							camera.shiftDist(xoff/w*160.f);
							camera.shiftZoom(yoff/h*160.f);
						}
						else {
							camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						}
						camera.updatePivotViewMat();
					}
					else {
						camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						camera.updateFreeViewMat();
					}
				}
				vp->cursor_pos_x = xPos; vp->cursor_pos_y = yPos;
			}
		}
		void mouse_btn_callback(int button, int action, int mods)
		{
			for( Viewport* vp : viewports ) {
				if( vp->hovered == false ) continue;
				Camera& camera = *vp->camera;
				if( action == GLFW_PRESS ) {
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					vp->cursor_pos_x = xpos;
					vp->cursor_pos_y = ypos;
				}
			}
		}
		void scroll_callback(double xoff, double yoff)
		{
			for( Viewport* vp : viewports ) {
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;

				camera.rotateCamera(xoff, yoff);
				camera.updatePivotViewMat();
				if( 0 ) {
					camera.shiftZoom(yoff*10.f);
					camera.updateProjMat();
				}
			}
		}
		void win_size_callback(int width, int height)
		{
			scr_width = width;
			scr_height = height;
		}
		void drop_callback(int count, const char** paths)
		{
			for( int i = 0; i < count; i++ ) {
				loadModel(paths[i]);
				break;
			}
		}
	};

}

#endif
