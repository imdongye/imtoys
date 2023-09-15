#include "app_simplification.h"
#include "simplify.h"
#include "map_baker.h"
#include <stb_image.h>
#include <stb_sprintf.h>
#include <limbrary/app_pref.h>
#include <limbrary/model_view/code_mesh.h>
#include <imgui.h>

namespace lim
{
	AppSimplification::AppSimplification() : AppBase(1200, 780, APP_NAME),
		light(), ground(code_mesh::genPlane(), "ground")
	{
		stbi_set_flip_vertically_on_load(true);

		programs.push_back(new Program("Normal Dot View", APP_DIR));
		programs.back()->attatch("pos_nor_uv.vs").attatch("ndv.fs").link();

		programs.push_back(new Program("Normal Dot Light", APP_DIR));
		programs.back()->attatch("pos_nor_uv.vs").attatch("ndl.fs").link();

		programs.push_back(new Program("Map View, bump", APP_DIR));
		programs.back()->attatch("uv_view.vs").attatch("map_view.fs").link();

		programs.push_back(new Program("Map View, my normal", APP_DIR));
		programs.back()->attatch("uv_view.vs").attatch("map_wnor.fs").link();

		programs.push_back(new Program("Map View, my normal + map_bump", APP_DIR));
		programs.back()->attatch("uv_view.vs").attatch("map_wnor_added_bump_map.fs").link();

		programs.push_back(new Program("Auto Normal", APP_DIR));
		programs.back()->attatch("pos_nor_uv.vs").attatch("auto_normal.fs").link();

		programs.push_back(new Program("Uv", APP_DIR));
		programs.back()->attatch("pos_nor_uv.vs").attatch("uv.fs").link();

		programs.push_back(new Program("Shadowed", APP_DIR));
		programs.back()->attatch("shadowed.vs").attatch("shadowed.fs").link();

		ground.program = programs[0];
		ground.meshes[0]->color = glm::vec3(0.8, 0.8, 0);
		ground.position = glm::vec3(0, 0, 0);
		ground.scale = glm::vec3(100, 100, 1);
		ground.orientation = glm::angleAxis(F_PI*0.5f, glm::vec3(1,0,0));
		ground.updateModelMat();

		addEmptyViewport();
		loadModel("assets/models/dwarf/Dwarf_2_Low.obj", 0);
		addEmptyViewport();
		loadModel("assets/models/dwarf/Dwarf_2_Low.obj", 1);
	}
	AppSimplification::~AppSimplification()
	{
		vpPackage.clear();
		for (Program *program : programs)
		{
			program->clear();
		}
	}
	void AppSimplification::addEmptyViewport()
	{
		char* vpName = fmtStrToBuf("viewport%d##simp", vpPackage.size);
		Viewport *viewport = new Viewport(vpName, new MsFramebuffer);
		viewport->framebuffer->clear_color = {0, 0, 1, 1};
		Scene *scene = new Scene();
		scene->lights.push_back(&light);
		AutoCamera *camera = new AutoCamera({0, 1, 8});
		vpPackage.push_back(viewport, scene, nullptr, camera);
	}
	void AppSimplification::loadModel(std::string_view path, int vpIdx)
	{
		Model* prevModel = vpPackage.models[vpIdx];
		if (prevModel != nullptr)
			delete prevModel;

		double start = glfwGetTime();
		Model* newModel = importModelFromFile(path.data(), true);
		if( !newModel ) return;

		newModel->program = programs[selectedProgIdx];

		Scene* curScene = vpPackage.scenes[vpIdx];
		if( prevModel!=nullptr )
			curScene->models.erase(std::find(curScene->models.begin(), curScene->models.end(), prevModel));
		curScene->models.push_back(newModel);
		vpPackage.models[vpIdx] = newModel;
		vpPackage.cameras[vpIdx]->pivot = newModel->position;
		vpPackage.cameras[vpIdx]->updatePivotViewMat();
		log::pure("Done! in %.3f sec.  \n", glfwGetTime() - start);
		AppPref::get().pushPathWithoutDup(path.data());
	}
	void AppSimplification::exportModel(size_t pIndex, int vpIdx)
	{
		Model *toModel = vpPackage.models[vpIdx];

		if (toModel == nullptr)
		{
			log::err("export\n");
			return;
		}

		double start = glfwGetTime();
		log::pure("Exporting %s.. .. ...... ...  .... .. . .... . .\n", toModel->name.c_str());

		exportModelToFile(exportPath, toModel, pIndex);

		log::pure("Done! in %.3f sec.  \n\n", glfwGetTime() - start);
	}
	void AppSimplification::simplifyModel(float lived_pct, int version, int agressiveness, bool verbose)
	{
		Model *fromModel = vpPackage.models[fromVpIdx];
		Model *toModel = vpPackage.models[toVpIdx];

		double start = glfwGetTime();
		log::pure("\nSimplifing %s..... . ... ... .. .. . .  .\n", fromModel->name.c_str());

		if (toModel != nullptr)
			delete toModel;

		toModel = fqms::simplifyModel(fromModel, lived_pct, version, agressiveness, verbose);
		int pct = 100.0 * toModel->nr_vertices / fromModel->nr_vertices;
		toModel->name += "_"+std::to_string(pct)+"_pct";

		Model* prevModel = vpPackage.models[toVpIdx];
		if (prevModel != nullptr)
			delete prevModel;
		vpPackage.models[toVpIdx] = toModel;
		Scene* curScene = vpPackage.scenes[toVpIdx];
		if( prevModel!=nullptr )
			curScene->models.erase(std::find(curScene->models.begin(), curScene->models.end(), prevModel));
		curScene->models.push_back(toModel);
		vpPackage.cameras[toVpIdx]->pivot = toModel->position;
		vpPackage.cameras[toVpIdx]->updatePivotViewMat();

		simp_time = glfwGetTime() - start;
		log::pure("Done! %d => %d in %.3f sec. \n\n", fromModel->nr_vertices, toModel->nr_vertices, simp_time);
	}
	// From: https://stackoverflow.com/questions/62007672/png-saved-from-opengl-framebuffer-using-stbi-write-png-is-shifted-to-the-right
	void AppSimplification::bakeNormalMap()
	{
		if (vpPackage.models[fromVpIdx] != nullptr && vpPackage.models[toVpIdx] != nullptr)
			MapBaker::bakeNormalMap(exportPath, vpPackage.models[fromVpIdx], vpPackage.models[toVpIdx]);
		else
			log::err("You Must to simplify before baking\n");
	}
	void AppSimplification::update()
	{
		if (isSameCamera)
		{
			Camera *firstCam = vpPackage.cameras[0];
			for (int i = 0; i < vpPackage.size; i++)
			{
				Framebuffer *fb = vpPackage.viewports[i]->framebuffer;
				vpPackage.scenes[i]->render(fb, firstCam);
			}
		}
		else
		{
			for (int i = 0; i < vpPackage.size; i++)
			{
				Framebuffer *fb = vpPackage.viewports[i]->framebuffer;
				Camera *cm = vpPackage.cameras[i];
				vpPackage.scenes[i]->render(fb, cm);
			}
		}
		/* render to screen */
		// scenes[0]->render(0, scr_width, scr_height, camera[0]);

		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void AppSimplification::renderImGui()
	{
		const Model *fromModel = vpPackage.models[fromVpIdx];
		const Model *toModel = vpPackage.models[toVpIdx];

		ImGui::DockSpaceOverViewport();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::BeginMenu("Import Recent"))
				{
					for (int i=0; i<vpPackage.viewports.size(); i++)
					{
						Viewport* vp = vpPackage.viewports[i];
						if (ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str()))
						{
							typename std::vector<std::string>::reverse_iterator iter;
							for (iter = AppPref::get().recent_model_paths.rbegin();
									iter != AppPref::get().recent_model_paths.rend(); iter++)
							{
								if (ImGui::MenuItem((*iter).c_str()))
								{
									loadModel((*iter), i);
									break;
								}
							}
							if (ImGui::MenuItem("Clear Recent"))
							{
								AppPref::get().clearData();
							}
							ImGui::EndMenu();
						}
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Export"))
				{
					for (int i = 0; i < vpPackage.size; i++)
					{
						Model *md = vpPackage.models[i];
						if (md == nullptr)
							continue;
						Viewport *vp = vpPackage.viewports[i];
						if (ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str()))
						{
							int nr_formats = getNrExportFormats();
							for (size_t pIndex = 0; pIndex < nr_formats; pIndex++)
							{
								const aiExportFormatDesc *format = getExportFormatInfo(pIndex);
								if (ImGui::MenuItem(format->id))
								{
									exportModel(pIndex, i);
								}
								if (ImGui::IsItemHovered())
								{
									static char exportFormatTooltipBuf[64];
									stbsp_sprintf(exportFormatTooltipBuf, "%s\n%s.%s", format->description, md->name.c_str(), format->fileExtension); // todo : caching
									ImGui::SetTooltip("%s",exportFormatTooltipBuf);
								}
							}
							ImGui::EndMenu();
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("add viewport"))
				{
					addEmptyViewport();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		/* 실행후 겹쳤을때 그리는순서에 따라서 위로오는게 결정됨 */
		// 중요한게 뒤로 오게 해야함

		for (int i = vpPackage.size - 1; i >= 0; i--)
		{
			vpPackage.viewports[i]->drawImGui();
		}

		log::drawViewer("log viwer##simplification");

		if (ImGui::Begin("Simplify Options##simp"))
		{
			ImGui::Text("From viewport:");
			for (int i = 0; i < vpPackage.size; i++)
			{
				int id = i;
				if (vpPackage.models[i] == nullptr)
					continue;
				ImGui::SameLine();
				if (ImGui::RadioButton((std::to_string(id) + "##1").c_str(), &fromVpIdx, id) && fromVpIdx == toVpIdx)
				{
					toVpIdx++;
					toVpIdx %= vpPackage.size;
				}
			}

			ImGui::Text("to viewport:");
			for( int i=0; i<vpPackage.size; i++ )
			{
				Viewport* vp = vpPackage.viewports[i];
				if (i == fromVpIdx)
					continue;
				ImGui::SameLine();
				if (ImGui::RadioButton((std::to_string(i) + "##2").c_str(), &toVpIdx, i))
				{
					log::pure("%s", i);
				}
			}

			// simplification
			static int pct = 80;
			ImGui::SliderInt("percent", &pct, 1, 100);
			static int selectedVersion = 1;
			const char *versionComboList[3] = {"agressiveness", "max_considered", "lossless"};
			ImGui::Combo("version", &selectedVersion, versionComboList, 3);
			static int agressiveness = 7;
			ImGui::SliderInt("agressiveness", &agressiveness, 5, 8);
			static bool verbose = true;
			ImGui::Checkbox("verbose", &verbose);
			if (ImGui::Button("Simplify") || simplifyTrigger)
			{
				simplifyTrigger = false;
				simplifyModel((float)pct / 100.f, selectedVersion, agressiveness, verbose);
			}

			ImGui::SameLine();
			ImGui::Text("target triangles = %d", static_cast<int>(fromModel->nr_triangles * pct));
			ImGui::Text("simplified in %lf sec", simp_time);
			ImGui::NewLine();

			// baking
			static int selectedTexSizeIdx = 2;
			static const int nrTexSize=6;
			static int texSizes[]={256, 512, 1024, 2048, 4096, 8192};
			static const char* texSizeStrs[]={"256", "512", "1024", "2048", "4096", "8192"};

			ImGui::Combo("texture resolution", &selectedTexSizeIdx, texSizeStrs, nrTexSize);

			if (ImGui::Button("Bake normal map") || bakeTrigger)
			{
				bakeTrigger = false;
				bakeNormalMap(texSizes[selectedTexSizeIdx]);
			}
		}
		ImGui::End();

		if (ImGui::Begin("Viewing Options##simp"))
		{
			ImGui::Text("<camera>");
			static int focusedCameraIdx = 0;
			static const char *vmode_strs[] = {"free", "pivot", "scroll"};
			int viewMode = 0;
			if (ImGui::Combo("mode", &viewMode, vmode_strs, sizeof(vmode_strs) / sizeof(char *)))
			{
				for (AutoCamera *cam : vpPackage.cameras)
				{
					cam->setViewMode(viewMode);
				}
			}
			ImGui::Checkbox("use same camera", &isSameCamera);
			ImGui::SliderFloat("move speed", &cameraMoveSpeed, 0.2f, 3.0f);
			for (int i = 0; i < vpPackage.size; i++)
			{ // for test
				if (vpPackage.viewports[i]->focused && focusedCameraIdx != i)
				{
					focusedCameraIdx = i;
				}
			}
			ImGui::Text("camera fov: %f", vpPackage.cameras[focusedCameraIdx]->fovy);
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));

			ImGui::Text("<shader>");
			std::vector<const char *> shaderList;
			for (Program *prog : programs)
				shaderList.push_back(prog->name.c_str());
			if (ImGui::Combo("type", &selectedProgIdx, shaderList.data(), shaderList.size()))
			{
				ground.program = programs[selectedProgIdx];
				for(Model* md : vpPackage.models)
				{
					md->program = programs[selectedProgIdx];
				}
			}
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));

			ImGui::Text("<light>");
			const float yawSpd = 360 * 0.001;
			if (ImGui::DragFloat("light yaw", &light.yaw, yawSpd, -10, 370, "%.3f"))
				light.updateMembers();
			const float pitchSpd = 70 * 0.001;
			if (ImGui::DragFloat("light pitch", &light.pitch, pitchSpd, -100, 100, "%.3f"))
				light.updateMembers();
			ImGui::Text("pos %f %f %f", light.position.x, light.position.y, light.position.z);

			static float bumpHeight = 100;
			if (ImGui::SliderFloat("bumpHeight", &bumpHeight, 0.0, 300.0))
			{
				for (auto &md : vpPackage.models)
				{
					if (md == nullptr)
						continue;
					md->bumpHeight = bumpHeight;
				}
			}
			static float texDelta = 0.00001;
			if (ImGui::SliderFloat("texDelta", &texDelta, 0.000001, 0.0001, "%f"))
			{
				for (auto &md : vpPackage.models)
				{
					if (md == nullptr)
						continue;
					md->texDelta = texDelta;
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin("Model Status##simp"))
		{
			static ImGuiTableFlags flags = ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable;

			if (ImGui::BeginTable("attributes table", vpPackage.viewports.size() + 1, flags, {0, ImGui::GetTextLineHeightWithSpacing() * 9}))
			{
				ImGui::TableSetupScrollFreeze(1, 1);
				ImGui::TableSetupColumn("attributes", ImGuiTableColumnFlags_NoHide);
				for (Viewport *vp : vpPackage.viewports)
				{
					ImGui::TableSetupColumn(vp->name.c_str());
				}
				ImGui::TableHeadersRow();

				int column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("name");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%s", md->name.c_str());
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("#verticies");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%d", md->nr_vertices);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("#triangles");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%d", md->nr_triangles);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("boundary_x");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%f", md->getBoundarySize().x);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("boundary_y");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%f", md->getBoundarySize().y);
					}
				}

				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("#meshes");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%d", (int)md->meshes.size());
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if (ImGui::TableSetColumnIndex(column++))
					ImGui::Text("#textures");
				for (Model *md : vpPackage.models)
				{
					if (ImGui::TableSetColumnIndex(column++))
					{
						if (md == nullptr)
							ImGui::Text("");
						else
							ImGui::Text("%d", (int)md->textures_loaded.size());
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();

		/* show texture */
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data)
											{
			data->DesiredSize.x = glm::max(data->DesiredSize.x, data->DesiredSize.y);
			data->DesiredSize.y = data->DesiredSize.x; });

		// todo
		if (MapBaker::bakedNormalMapPointer != nullptr)
		{
			if (ImGui::Begin("Baked Normal Map##simp"))
			{
				ImVec2 vMin = ImGui::GetWindowContentRegionMin();
				ImVec2 vMax = ImGui::GetWindowContentRegionMax();
				glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
				// ImGui::Text("%f %f", rectSize.x, rectSize.y);
				const float minLength = glm::min(rectSize.x, rectSize.y);
				GLuint rtId = MapBaker::bakedNormalMapPointer->getRenderedTex();
				ImGui::Image(INT2VOIDP(rtId), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
			}
			ImGui::End();
		}

		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data)
											{
			data->DesiredSize.x = glm::max(data->DesiredSize.x, data->DesiredSize.y);
			data->DesiredSize.y = data->DesiredSize.x; });

		if (ImGui::Begin("shadowMap##simp") && light.shadow_enabled)
		{
			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();
			glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
			// ImGui::Text("%f %f", rectSize.x, rectSize.y);
			const float minLength = glm::min(rectSize.x, rectSize.y);
			ImGui::Image(INT2VOIDP(light.map_Shadow.getRenderedTex()), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}
	void AppSimplification::keyCallback(int key, int scancode, int action, int mods)
	{
		if ((GLFW_MOD_CONTROL == mods) && (GLFW_KEY_S == key))
		{
			simplifyTrigger = true;
			bakeTrigger = true;
		}
		if ((GLFW_MOD_CONTROL == mods) && (GLFW_KEY_E == key))
		{
			exportModel(3, lastFocusedVpIdx);
		}
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
	void AppSimplification::mouseBtnCallback(int button, int action, int mods)
	{
		// 이전에 선택된 viewport 저장
		if (vpPackage.viewports[lastFocusedVpIdx]->hovered == false)
		{
			for (int i = 0; i < vpPackage.size; i++)
			{
				if (vpPackage.viewports[i]->hovered)
					lastFocusedVpIdx = i;
			}
		}
	}
	void AppSimplification::dndCallback(int count, const char **paths)
	{
		int selectedVpIdx = -1;
		double xPos, yPos;
		int winX, winY, gX, gY;
		glfwGetCursorPos(window, &xPos, &yPos);
		glfwGetWindowPos(window, &winX, &winY);

		gX = winX + xPos;
		gY = winY + yPos;
		for (int i = 0; i < vpPackage.size; i++)
		{
			if (vpPackage.viewports[i]->hovered)
			{
				selectedVpIdx = i;
			}
		}
		if (selectedVpIdx >= 0)
		{
			loadModel(paths[0], selectedVpIdx);
		}
		else
		{
			log::err("you must drop to viewport\n");
		}
	}
}