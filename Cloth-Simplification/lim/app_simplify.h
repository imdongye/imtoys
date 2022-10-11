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
		enum class VIEWING_MODE : unsigned int
		{
			PIVOT, FREE, SCROLL
		};
		VIEWING_MODE viewingMode=VIEWING_MODE::PIVOT;
		bool isSameCamera = false;
		float cameraMoveSpeed=1.6;
		Light light = Light();

		int fromViewportIdx=0;
		int toViewportIdx=1;
		ViewportPackage vpPackage;

		int selectedProgIdx=0;
		std::vector<Program*> programs;

		const char* exportPath = "result/";
		int selectedBakeSizeIdx=2;
		std::vector<const char*> bakeSizeList={"256", "512", "1024", "2048", "4096", "8192"};
		Framebuffer bakedNormalMap=Framebuffer();
		Program bakerProg=Program("Normal Map Baker");
	public:
		SimplifyApp(): AppBase(1280, 720)
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
			programs.back()->attatch("pos_nor_uv.vs").attatch("bump_map.fs").link();

			programs.push_back(new Program("Normal"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("normal_map.fs").link();

			programs.push_back(new Program("Shadowed"));
			programs.back()->attatch("shadowed.vs").attatch("shadowed.fs").link();

			programs.push_back(new Program("Uv View"));
			programs.back()->attatch("uv_view.vs").attatch("uv_view.fs").link();

			addEmptyViewport();
			loadModel("archive/dwarf/Dwarf_2_Low.obj", 0);
			addEmptyViewport();

			init_callback();
			imgui_modules::initImGui(window);
		}
		~SimplifyApp()
		{
			vpPackage.clear();
			for( Program* program : programs ) {
				program->clear();
			}
			imgui_modules::destroyImGui();
		}
	private:
		void addEmptyViewport()
		{
			Viewport* viewport = new Viewport();
			Scene* scene = new Scene(light);
			Camera* camera = new Camera(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0, 0, -1), 1);
			vpPackage.push_back(viewport, scene, nullptr, camera);
		}
		void loadModel(std::string_view path, int vpIdx)
		{
			Model* temp = vpPackage.models[vpIdx];
			if( temp!=nullptr ) {
				delete temp;
			}

			double start = glfwGetTime();
			Logger::get(1).log("Loading %s..... . ... ..... .. .... .. . . .. .\n", path.data());
			try {
				temp = ModelLoader::loadFile(path.data());
				temp->program = programs[selectedProgIdx];
			} catch( const char* error_str ) {
				Logger::get()<<error_str<<Logger::endl;
			}
			vpPackage.scenes[vpIdx]->setModel(temp);
			vpPackage.models[vpIdx] = temp;
			vpPackage.cameras[vpIdx]->pivot = temp->position;

			Logger::get().log("Done! in %.3f sec.  \n", glfwGetTime() - start);
			AppPref::get().pushPathWithoutDup(path.data());
		}
		void exportModel(size_t pIndex, int vpIdx)
		{
			Model *toModel = vpPackage.models[vpIdx];

			if( toModel == nullptr ) {
				Logger::get()<<"error : export"<<Logger::endl;
				return;
			}

			double start = glfwGetTime();
			Logger::get().log("Exporting %s.. .. ...... ...  .... .. . .... . .\n", toModel->name.c_str());

			ModelExporter::exportModel(exportPath, toModel, pIndex);

			Logger::get().log("Done! in %.3f sec.  \n\n", glfwGetTime()-start);
		}
		void simplifyModel(float lived_pct = 0.8f
						   , int version = 0, int agressiveness=7, bool verbose=true)
		{
			Model* fromModel= vpPackage.models[fromViewportIdx];
			Model* toModel= vpPackage.models[toViewportIdx];

			double start = glfwGetTime();
			Logger::get().log("\nSimplifing %s..... . ... ... .. .. . .  .\n", fromModel->name.c_str());

			if( toModel != nullptr ) delete toModel;

			toModel = fqms::simplifyModel(fromModel, lived_pct, version, agressiveness, verbose);
			int pct = 100.0*toModel->verticesNum/fromModel->verticesNum;
			toModel->name+=fmToStr("_%d_pct", pct);
			vpPackage.models[toViewportIdx] = toModel;
			vpPackage.scenes[toViewportIdx]->setModel(toModel);
			vpPackage.cameras[toViewportIdx]->pivot = toModel->position;

			double simpTime = glfwGetTime() - start;
			Logger::get().simpTime = simpTime;
			Logger::get().log("Done! %d => %d in %.3f sec. \n\n"
							  , fromModel->verticesNum, toModel->verticesNum, simpTime);
		}
		// From: https://stackoverflow.com/questions/62007672/png-saved-from-opengl-framebuffer-using-stbi-write-png-is-shifted-to-the-right
		void bakeNormalMap()
		{
			Model *oriMod = vpPackage.models[fromViewportIdx];
			Model *toMod = vpPackage.models[toViewportIdx];
			std::map<std::string, std::vector<Mesh*>> mergeByNormalMap;
			GLuint pid = bakerProg.use();
			GLuint w = bakedNormalMap.width;
			GLuint h = bakedNormalMap.height;
			static GLubyte* data = new GLubyte[3*w*h];

			for( Mesh* mesh : oriMod->meshes ) {
				for( Texture& tex : mesh->textures ) {
					if( tex.type == "map_Bump" ) {
						mergeByNormalMap[tex.path].push_back(mesh);
					}
				}
			}
			for( auto&[filepath, meshes] : mergeByNormalMap ) {
				std::string fullPath(exportPath);
				fullPath += oriMod->name+"/"+ filepath;
				Logger::get()<<fullPath<<Logger::endl;
				bakedNormalMap.bind();
				for( Mesh* mesh : meshes ) {
					mesh->draw(pid);
				}
				bakedNormalMap.unbind();

				glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// 덮어쓴다.
				stbi_write_png(fullPath.c_str(), w, h, 3, data, w*3);
			}

			Logger::get()<<"done"<<Logger::endll;
		}
	private:
		virtual void update() final
		{
			processInput();

			//scenes[0]->render(0, scr_width, scr_height, cameras[0]); 

			if( isSameCamera ) {
				Camera* firstCam = vpPackage.cameras[0];
				for( int i=0; i<vpPackage.size; i++ ) {
					Framebuffer* fb = vpPackage.viewports[i]->framebuffer;
					vpPackage.scenes[i]->render(fb, firstCam);
				}
			}
			else {
				for( int i=0; i<vpPackage.size; i++ ) {
					Framebuffer* fb = vpPackage.viewports[i]->framebuffer;
					Camera* cm = vpPackage.cameras[i];
					vpPackage.scenes[i]->render(fb, cm);
				}
			}

			renderImGui();

			/* render to screen */
			//scenes[0]->render(0, scr_width, scr_height, camera[0]);
		}
		void renderImGui()
		{
			const Model* fromModel = vpPackage.models[fromViewportIdx];
			const Model* toModel = vpPackage.models[toViewportIdx];

			imgui_modules::beginImGui();

			imgui_modules::ShowExampleAppDockSpace([&]() {
				if( ImGui::BeginMenu("File") ) {
					if( ImGui::BeginMenu("Open Recent") ) {
						for( Viewport* vp : vpPackage.viewports ) {
							if( ImGui::BeginMenu(("Viewport"+std::to_string(vp->id)).c_str()) ) {
								typename std::vector<std::string>::reverse_iterator iter;
								for( iter = AppPref::get().recentModelPaths.rbegin();
									iter != AppPref::get().recentModelPaths.rend(); iter++ ) {
									if( ImGui::MenuItem((*iter).c_str()) ) {
										loadModel((*iter), vp->id);
										break;
									}
								}
								if( ImGui::MenuItem("Clear Recent") ) {
									AppPref::get().clearData();
								}
								ImGui::EndMenu();
							}
						}
						ImGui::EndMenu();
					}
					if( ImGui::BeginMenu("Export") ) {
						for( int i=0; i<vpPackage.size; i++ ) {
							Model* md = vpPackage.models[i];
							if( md==nullptr ) continue;
							Viewport* vp = vpPackage.viewports[i];
							if( ImGui::BeginMenu(("Viewport"+std::to_string(vp->id)).c_str()) ) {
								for( size_t pIndex=0; pIndex<ModelExporter::nr_formats; pIndex++ ) {
									const aiExportFormatDesc* format = ModelExporter::getFormatInfo(pIndex);
									if( ImGui::MenuItem(format->id) ) {
										exportModel(pIndex, i);
									}
									if( ImGui::IsItemHovered() ) {
										ImGui::SetTooltip(fmToStr("%s\n%s.%s", format->description, md->name.c_str(), format->fileExtension).c_str());
									}
								}
								ImGui::EndMenu();
							}
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if( ImGui::BeginMenu("View") ) {
					if( ImGui::MenuItem("add viewport") ) {
						addEmptyViewport();
					}
					ImGui::EndMenu();
				}
			});

			ImGui::ShowDemoWindow();

			/* 실행후 겹쳤을때 그리는순서에 따라서 위로오는게 결정됨 */
			// 중요한게 뒤로 오게 해야함

			for( int i=vpPackage.size-1; i>=0; i-- ) {
				vpPackage.viewports[i]->drawImGui();
			}

			Logger::get().drawImGui();

			if( ImGui::Begin("Simplify Options") ) {
				ImGui::Text("From viewport:");
				for( int i=0; i<vpPackage.size; i++ ) {
					int id = vpPackage.viewports[i]->id;
					if( vpPackage.models[i]==nullptr ) continue;
					ImGui::SameLine();
					if( ImGui::RadioButton((std::to_string(id)+"##1").c_str(), &fromViewportIdx, id)
					   && fromViewportIdx == toViewportIdx ) {
						toViewportIdx++;
						toViewportIdx%=vpPackage.size;
					}
				}

				ImGui::Text("to viewport:");
				for( Viewport* vp : vpPackage.viewports ) {
					if( vp->id == fromViewportIdx ) continue;
					ImGui::SameLine();
					if( ImGui::RadioButton((std::to_string(vp->id)+"##2").c_str(), &toViewportIdx, vp->id) ) {
						Logger::get()<<vp->id;
					}
				}

				// simplification
				static  int pct = 80;
				ImGui::SliderInt("percent", &pct, 1, 100);
				static int selectedVersion = 1;
				const char* versionComboList[3] ={"agressiveness", "max_considered", "lossless"};
				ImGui::Combo("version", &selectedVersion, versionComboList, 3);
				static int agressiveness = 7;
				ImGui::SliderInt("agressiveness", &agressiveness, 5, 8);
				static bool verbose = true;
				ImGui::Checkbox("verbose", &verbose);
				if( ImGui::Button("Simplify") ) {
					simplifyModel((float)pct/100.f, selectedVersion, agressiveness, verbose);
				}

				ImGui::SameLine();
				ImGui::Text("target triangles = %d", static_cast<int>(fromModel->trianglesNum*pct));
				ImGui::Text("simplified in %lf sec", Logger::get().simpTime);
				ImGui::NewLine();

				// baking
				if( ImGui::Combo("texture resolution", &selectedBakeSizeIdx, bakeSizeList.data(), bakeSizeList.size()) ) {
					bakedNormalMap.resize(atoi(bakeSizeList[selectedBakeSizeIdx]));
				}
				if( ImGui::Button("Bake normal map") ) {
					bakeNormalMap();
				}

			} ImGui::End();


			if( ImGui::Begin("Viewing Options") ) {
				ImGui::Text("<camera>");
				static int focusedCameraIdx=0;
				static const char* vmode_strs[] ={"pivot","free","scroll"};
				static int tempVmode = static_cast<int>(viewingMode);
				if( ImGui::Combo("mode", &tempVmode, vmode_strs, sizeof(vmode_strs)/sizeof(char*)) ) {
					viewingMode = static_cast<VIEWING_MODE>(tempVmode);
				}
				ImGui::Checkbox("use same camera", &isSameCamera);
				ImGui::SliderFloat("move speed", &cameraMoveSpeed, 0.2f, 3.0f);
				for( int i=0; i<vpPackage.size; i++ ) { // for test
					if( vpPackage.viewports[i]->focused && focusedCameraIdx!=i) {
						focusedCameraIdx = i;
					}
				}
				ImGui::Text("camera fov: %f", vpPackage.cameras[focusedCameraIdx]->fovy);
				ImGui::Dummy(ImVec2(0.0f, 8.0f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.0f, 3.0f));
				ImGui::Text("<shader>");
				static int shaderTargetIdx=0;
				std::vector<const char*> stargetList;
				stargetList.push_back("All");
				for( Viewport* vp : vpPackage.viewports )
					stargetList.push_back(vp->name.c_str());
				if( ImGui::Combo("target", &shaderTargetIdx, stargetList.data(), stargetList.size()) ) {
					if( shaderTargetIdx==0 ) {
						for( auto& sc : vpPackage.scenes ) {
							sc->ground->program = programs[selectedProgIdx];
							if( sc->model == nullptr ) continue;
							sc->model->program = programs[selectedProgIdx];
						}
					}
					else {
						Model* md = vpPackage.models[shaderTargetIdx-1];
						if( md != nullptr ) md->program = programs[selectedProgIdx];
					}
				}
				std::vector<const char*> shaderList;
				for( Program* prog : programs )
					shaderList.push_back(prog->name.c_str());
				if( ImGui::Combo("type", &selectedProgIdx, shaderList.data(), shaderList.size()) ) {
					if( shaderTargetIdx==0 ) {
						for( auto& sc : vpPackage.scenes ) {
							sc->ground->program = programs[selectedProgIdx];
							if( sc->model == nullptr ) continue;
							sc->model->program = programs[selectedProgIdx];
						}
					}
					else {
						Model* md = vpPackage.models[shaderTargetIdx-1];
						if( md != nullptr ) md->program = programs[selectedProgIdx];
					}
				}
				ImGui::Dummy(ImVec2(0.0f, 8.0f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.0f, 3.0f));
				ImGui::Text("<light>");
				const float yawSpd = 360*0.001;
				if( ImGui::DragFloat("light yaw", &light.yaw, yawSpd, -10, 370, "%.3f") )
					light.updateMembers();
				const float pitchSpd = 70*0.001;
				if( ImGui::DragFloat("light pitch", &light.pitch, pitchSpd, -100, 100, "%.3f") )
					light.updateMembers();

				ImGui::Text("pos %f %f %F", light.position.x, light.position.y, light.position.z);
			} ImGui::End();


			if( ImGui::Begin("Model Status") ) {
				static ImGuiTableFlags flags =  ImGuiTableFlags_ContextMenuInBody |ImGuiTableFlags_ScrollX| ImGuiTableFlags_ScrollY
					| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable;

				if( ImGui::BeginTable("attributes table", vpPackage.viewports.size()+1, flags, {0,ImGui::GetTextLineHeightWithSpacing()*9}) ) {
					ImGui::TableSetupScrollFreeze(1, 1);
					ImGui::TableSetupColumn("attributes", ImGuiTableColumnFlags_NoHide);
					for( Viewport* vp : vpPackage.viewports ) {
						ImGui::TableSetupColumn(vp->name.c_str());
					}
					ImGui::TableHeadersRow();


					int column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("name");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%s", md->name.c_str());
						}
					}
					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("#verticies");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr )  ImGui::Text("");
							else ImGui::Text("%d", md->verticesNum);
						}
					}
					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("#triangles");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%d", md->trianglesNum);
						}
					}
					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("boundary_x");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%f", md->getBoundarySize().x);
						}
					}
					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("boundary_y");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%f", md->getBoundarySize().y);
						}
					}

					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("#meshes");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%d", md->meshes.size());
						}
					}
					column = 0;
					ImGui::TableNextRow();
					if( ImGui::TableSetColumnIndex(column++) )
						ImGui::Text("#textures");
					for( Model* md : vpPackage.models ) {
						if( ImGui::TableSetColumnIndex(column++) ) {
							if( md==nullptr ) ImGui::Text("");
							else ImGui::Text("%d", md->textures_loaded.size());
						}
					}
					ImGui::EndTable();
				}
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
			for( int i=0; i<vpPackage.size; i++ ) {
				Viewport& vp = *(vpPackage.viewports[i]);
				if( vp.dragging == false ) continue;
				Camera& camera = *(vpPackage.cameras[i]);
				switch( viewingMode ) {
				case VIEWING_MODE::FREE:
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
					camera.updateFreeViewMat();
					break;
				}
				break;
			}
		}
		void key_callback(int key, int scancode, int action, int mode)
		{
			if( key==GLFW_KEY_TAB && action==GLFW_PRESS ) {
				//viewingMode++;
			}
			if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS ) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
		}
		void cursor_pos_callback(double xPos, double yPos)
		{
			static const double pivotRotSpd = 0.3;
			static const double pivotShiftSpd = -0.003;
			static const double freeRotSpd = 0.3;
			static double xOld, yOld;
			static double xOff, yOff;
			xOff = xPos-xOld;
			yOff = yOld-yPos;

			for( int i=0; i<vpPackage.size; i++ ) {
				Viewport& vp = *(vpPackage.viewports[i]);
				if( vp.dragging == false ) continue;
				Camera& camera = *(vpPackage.cameras[(isSameCamera)?0:i]);

				switch( viewingMode ) {
				case VIEWING_MODE::PIVOT:
					if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS ) {
						camera.shiftPos(xOff*pivotShiftSpd, yOff*pivotShiftSpd);
						camera.updatePivotViewMat();
					}
					else {
						camera.rotateCamera(xOff*pivotRotSpd, yOff*pivotRotSpd);
						camera.updatePivotViewMat();
					}
					break;
				case VIEWING_MODE::FREE:
					camera.rotateCamera(xOff*freeRotSpd, yOff*freeRotSpd);
					camera.updateFreeViewMat();
					break;
				case VIEWING_MODE::SCROLL:
					break;
				}
				break;
			}
			xOld = xPos; yOld = yPos;
		}
		void mouse_btn_callback(int button, int action, int mods)
		{
		}
		void scroll_callback(double xoff, double yoff)
		{
			static const double rotateSpd = 1.7;
			for( int i=0; i<vpPackage.size; i++ ) {
				Viewport& vp = *(vpPackage.viewports[i]);
				if( !vp.hovered ) continue;
				Camera& camera = *(vpPackage.cameras[(isSameCamera)?0:i]);

				switch( viewingMode ) {
				case VIEWING_MODE::PIVOT:
					camera.shiftDist(yoff*3.f);
					camera.updatePivotViewMat();
					break;
				case VIEWING_MODE::FREE:
					camera.shiftZoom(yoff*5.f);
					camera.updateProjMat();
					break;
				case VIEWING_MODE::SCROLL:
					camera.rotateCamera(xoff*rotateSpd, yoff*rotateSpd);
					camera.updatePivotViewMat();
					break;
				}
				break;
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
				int hoveredIdx=0;
				for( Viewport* vp : vpPackage.viewports ) {
					if( vp->hovered==true )
						break;
					hoveredIdx++;
				}
				if( hoveredIdx>=vpPackage.size ) {
					Logger::get()<<"[error] you must drop to viewport"<<Logger::endl;
					return;
				}

				loadModel(paths[i], hoveredIdx);
				break;
			}
		}
	};

}

#endif
