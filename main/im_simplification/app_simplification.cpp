/*

모델을 교체할때 scene.models[1]도 수정해줘야함

*/

#include "app_simplification.h"
#include "simplify.h"
#include <stb_image.h>
#include <stb_sprintf.h>
#include <limbrary/app_pref.h>
#include <limbrary/model_view/code_mesh.h>
#include <imgui.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>

namespace lim
{
	AppSimplification::AppSimplification() : AppBase(1200, 780, APP_NAME),
		light(), ground("ground")
	{
		stbi_set_flip_vertically_on_load(true);

		programs.push_back(new Program("Normal Dot View"));
		programs.back()->attatch("mvp.vs").attatch("ndv.fs").link();

		programs.push_back(new Program("Normal Dot Light"));
		programs.back()->attatch("mvp.vs").attatch("ndl.fs").link();

		programs.push_back(new Program("Map View, my normal", APP_DIR));
		programs.back()->attatch("uv_view.vs").attatch("wnor_out.fs").link();

		programs.push_back(new Program("Auto Normal", APP_DIR));
		programs.back()->attatch("assets/shaders/mvp.vs").attatch("bumped.fs").link();

		programs.push_back(new Program("bitoper", APP_DIR));
		programs.back()->attatch("assets/shaders/mvp.vs").attatch("bitoper.fs").link();

		programs.push_back(new Program("Shadowed", APP_DIR));
		programs.back()->attatch("shadowed.vs").attatch("shadowed.fs").link();
		for( Program* prog:programs )
			shader_names.push_back(prog->name.c_str());

		AssetLib::get().default_mat->prog = programs[0];

		ground.meshes.push_back(code_mesh::genPlane());
		ground.root.meshes.push_back(ground.meshes.back());
		ground.materials.push_back(new Material());
		ground.materials.back()->Kd = {0,1,0,1};
		ground.meshes.back()->material = ground.materials.back();

		ground.position = {0, 0, 0};
		ground.scale = {100, 1, 100};
		ground.updateModelMat();

		addEmptyViewport();
		doLoadModel("assets/models/dwarf/Dwarf_2_Low.obj", 0);
		addEmptyViewport();
	}
	AppSimplification::~AppSimplification()
	{
		for( auto prog : programs ) {
			delete prog;
		}
		for( auto vp : viewports ) {
			delete vp;
		}
		for( auto md : models ) {
			delete md;
		}
	}
	void AppSimplification::addEmptyViewport()
	{
		const char* vpName = fmtStrToBuf("viewport%d##simp", nr_viewports);
		ViewportWithCamera* vp = new ViewportWithCamera(vpName, new MsFramebuffer);
		vp->camera.shiftPos({1,1,0});
		vp->framebuffer->clear_color = {0, 0, 1, 1};
		viewports.push_back(vp);

		Model* md = new Model();
		models.push_back(md);

		scenes.push_back({});
		Scene& scn = scenes.back();
		scn.models.push_back(&ground);
		scn.models.push_back(md); // scene의 두번째 모델이 타겟 모델
		scn.lights.push_back(&light);
		nr_viewports++;
	}
	void AppSimplification::doLoadModel(std::string_view path, int vpIdx)
	{
		if( vpIdx<0||vpIdx>=nr_viewports ) {
			log::err("wrong vpIdx in load model");
			return;
		}
		delete models[vpIdx];

		double elapsedTime = glfwGetTime();
		models[vpIdx] = importModelFromFile(path.data(), true, true);
		if( models[vpIdx]==nullptr ) {
			return;
		}
		elapsedTime = glfwGetTime() - elapsedTime;
		log::pure("Done! in %.3f sec.  \n", elapsedTime);

		Model& md = *models[vpIdx];
		md.position = {0,md.pivoted_scaled_bottom_height, 0};
		md.updateModelMat();

		Scene& scn = scenes[vpIdx];
		scn.models[1] = models[vpIdx]; // 삭제된모델에서 교체

		auto& vp = *viewports[vpIdx];
		vp.camera.pivot = md.position;
		vp.camera.position.y = md.position.y;
		vp.camera.updateFromPosAndPivot();

		AppPref::get().saveRecentModelPath(path.data());
	}
	void AppSimplification::doExportModel(size_t pIndex, int vpIdx)
	{
		Model* toModel = models[vpIdx];

		if( toModel == nullptr ) {
			log::err("export\n");
			return;
		}

		double start = glfwGetTime();
		log::pure("Exporting %s.. .. ...... ...  .... .. . .... . .\n", toModel->name.c_str());

		exportModelToFile(toModel, pIndex);

		log::pure("Done! in %.3f sec.  \n\n", glfwGetTime() - start);
	}
	void AppSimplification::doSimplifyModel(float lived_pct, int version, int agressiveness, bool verbose)
	{
		Model* fromMd = models[from_vp_idx];
		Model* toMd = models[to_vp_idx];

		if( toMd != nullptr )
			delete toMd;
		toMd = fromMd->clone();
		models[to_vp_idx] = toMd;
		scenes[to_vp_idx].models[1] = toMd;

		simplifyModel(*toMd, lived_pct, version, agressiveness, verbose);
		int pct = 100.0 * toMd->nr_vertices / fromMd->nr_vertices;
		toMd->name += "_"+std::to_string(pct)+"_pct";
		toMd->path = std::string(export_path)+"/"+toMd->name;

		viewports[to_vp_idx]->camera.pivot = toMd->position;
		viewports[to_vp_idx]->camera.updateFromPosAndPivot();
	}
	// From: https://stackoverflow.com/questions/62007672/png-saved-from-opengl-framebuffer-using-stbi-write-png-is-shifted-to-the-right
	void AppSimplification::doBakeNormalMap(int texSize)
	{
		if( models[from_vp_idx] != nullptr && models[to_vp_idx] != nullptr )
			bakeNormalMap(models[from_vp_idx], models[to_vp_idx], texSize);
		else
			log::err("You Must to simplify before baking\n");
	}



	void AppSimplification::update()
	{
		if( is_same_camera )
		{
			Camera& cam = viewports[last_focused_vp_idx]->camera; 
			float backupAspect = cam.aspect;
			for( int i=0; i<nr_viewports; i++ ) {
				cam.aspect = viewports[i]->camera.aspect;
				cam.updateProjMat();
				render(*viewports[i]->framebuffer, cam, scenes[i]);
			}
			cam.aspect = backupAspect;
			cam.updateProjMat();
		}
		else
		{
			for( int i=0; i<nr_viewports; i++ )
				render(*viewports[i]->framebuffer, viewports[i]->camera, scenes[i]);
		}

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
		const Model& fromMd = *models[from_vp_idx];
		const Model& toMd = *models[to_vp_idx];

		ImGui::DockSpaceOverViewport();

		if( ImGui::BeginMainMenuBar() )
		{
			if( ImGui::BeginMenu("File") )
			{
				if( ImGui::BeginMenu("Import Recent") )
				{
					for( int i=0; i<nr_viewports; i++ )
					{
						if( ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str())) 
						{
							typename std::vector<std::string>::reverse_iterator iter;
							for( iter = AppPref::get().recent_model_paths.rbegin();
									iter != AppPref::get().recent_model_paths.rend(); iter++ )
							{
								if( ImGui::MenuItem((*iter).c_str()) )
								{
									doLoadModel((*iter), i);
									break;
								}
							}
							if(ImGui::MenuItem("Clear Recent") )
							{
								AppPref::get().clearData();
							}
							ImGui::EndMenu();
						}
					}
					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("Export") )
				{
					for( int i = 0; i < nr_viewports; i++ )
					{
						const Model& md = *models[i];
						if( md.meshes.size()==0 )
							continue;

						if( ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str()) )
						{
							int nr_formats = getNrExportFormats();
							for( size_t pIndex = 0; pIndex < nr_formats; pIndex++ )
							{
								const aiExportFormatDesc *format = getExportFormatInfo(pIndex);
								if( ImGui::MenuItem(format->id) )
								{
									doExportModel(pIndex, i);
								}
								if( ImGui::IsItemHovered() )
								{
									static char exportFormatTooltipBuf[64];
									stbsp_sprintf(exportFormatTooltipBuf, "%s\n%s.%s", format->description, md.name.c_str(), format->fileExtension); // todo : caching
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
			if( ImGui::BeginMenu("View") )
			{
				if( ImGui::MenuItem("add viewport") )
				{
					addEmptyViewport();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// 실행후 겹쳤을때 그리는순서에 따라서 위로오는게 결정됨, 방금 생성한게 위에 오기하기위함.
		for( int i=nr_viewports-1; i>=0; i-- )
		{
			viewports[i]->drawImGui();
		}

		log::drawViewer("log viwer##simplification");

		if( ImGui::Begin("Simplify Options##simp") )
		{
			ImGui::Text("From viewport:");
			for( int i=0; i<nr_viewports; i++ )
			{
				if( models[i]->meshes.size() == 0 )
					continue;
				ImGui::SameLine();
				if( ImGui::RadioButton( fmtStrToBuf("%d##1", i), &from_vp_idx, i) && from_vp_idx == to_vp_idx ) {
					to_vp_idx++;
					to_vp_idx %= nr_viewports;
				}
			}

			ImGui::Text("to viewport:");
			for( int i=0; i<nr_viewports; i++ )
			{
				if( i == from_vp_idx )
					continue;
				ImGui::SameLine();
				if( ImGui::RadioButton((std::to_string(i) + "##2").c_str(), &to_vp_idx, i) )
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
			static double simpTime = 0.0;
			if( ImGui::Button("Simplify") || simplify_trigger )
			{
				log::pure("\nSimplifing %s..... . ... ... .. .. . .  .\n", fromMd.name.c_str());
				simpTime = glfwGetTime();
				simplify_trigger = false;
				doSimplifyModel((float)pct / 100.f, selectedVersion, agressiveness, verbose);
				simpTime = glfwGetTime()-simpTime;
				log::pure("Done! %d => %d in %.3f sec. \n\n", fromMd.nr_vertices, toMd.nr_vertices, simpTime);
			}

			ImGui::SameLine();
			ImGui::Text("target triangles = %d", (int)(fromMd.nr_triangles * pct));
			ImGui::Text("simplified in %lf sec", simpTime);
			ImGui::NewLine();

			// baking
			static int selectedTexSizeIdx = 2;
			static const int nrTexSize=6;
			static int texSizes[]={256, 512, 1024, 2048, 4096, 8192};
			static const char* texSizeStrs[]={"256", "512", "1024", "2048", "4096", "8192"};

			ImGui::Combo("texture resolution", &selectedTexSizeIdx, texSizeStrs, nrTexSize);

			if( ImGui::Button("Bake normal map") || bake_trigger )
			{
				bake_trigger = false;
				doBakeNormalMap(texSizes[selectedTexSizeIdx]);
			}
		}
		ImGui::End();

		if( ImGui::Begin("Viewing Options##simp") )
		{
			ImGui::Text("<camera>");
			static int focusedCameraIdx = 0;
			static const char *vmode_strs[] = {"free", "pivot", "scroll"};
			int viewMode = 0;
			if( ImGui::Combo("mode", &viewMode, vmode_strs, sizeof(vmode_strs) / sizeof(char *)) ) {
				viewports[last_focused_vp_idx]->camera.setViewMode(viewMode);
			}
			ImGui::Checkbox("use same camera", &is_same_camera);
			static float cameraMoveSpeed = 4.0f;
			if(ImGui::SliderFloat("move speed", &cameraMoveSpeed, 2.0f, 6.0f)) {
				viewports[last_focused_vp_idx]->camera.move_free_spd = cameraMoveSpeed;
			}
			ImGui::Text("camera fov: %f", viewports[last_focused_vp_idx]->camera.fovy);
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));

			ImGui::Text("<shader>");
			if( ImGui::Combo("type", &selected_prog_idx, shader_names.data(), shader_names.size()) )
			{
				AssetLib::get().default_mat->prog = programs[selected_prog_idx];
			}
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));

			ImGui::Text("<light>");
			const static float litThetaSpd = 70 * 0.001;
			const static float litPiSpd = 360 * 0.001;
			static float litTheta = 30.f;
			static float litPi = 30.f;
			if( ImGui::DragFloat("light yaw", &litPi, litPiSpd, 0, 360, "%.3f") ||
				ImGui::DragFloat("light pitch", &litTheta, litThetaSpd, 0, 180, "%.3f") ) {

				light.setRotate(litTheta, litPi);
			}
			ImGui::Text("pos %f %f %f", light.position.x, light.position.y, light.position.z);

			static float bumpHeight = 100;
			if( ImGui::SliderFloat("bumpHeight", &bumpHeight, 0.0, 300.0) ) {
				for( Model* md : models ) {
					for( Material* mat : md->materials ) {
						mat->bumpHeight = bumpHeight;
					}
				}
			}
			static float texDelta = 0.00001;
			if( ImGui::SliderFloat("texDelta", &texDelta, 0.000001, 0.0001, "%f") ) {
				for( Model* md : models ) {
					for( Material* mat : md->materials ) {
						mat->texDelta = texDelta;
					}
				}
			}
		}
		ImGui::End();

		if( ImGui::Begin("Model Status##simp") )
		{
			static ImGuiTableFlags flags = ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable;

			if( ImGui::BeginTable("attributes table", nr_viewports + 1, flags, {0, ImGui::GetTextLineHeightWithSpacing() * 9}) )
			{
				ImGui::TableSetupScrollFreeze(1, 1);
				ImGui::TableSetupColumn("attributes", ImGuiTableColumnFlags_NoHide);
				for( Viewport* vp : viewports )
				{
					ImGui::TableSetupColumn(vp->name.c_str());
				}
				ImGui::TableHeadersRow();

				int column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("name");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%s", md->name.c_str());
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("#verticies");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%d", md->nr_vertices);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("#triangles");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%d", md->nr_triangles);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("boundary_x");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%f", md->boundary_size.x);
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("boundary_y");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%f", md->boundary_size.y);
					}
				}

				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("#meshes");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
							ImGui::Text("");
						else
							ImGui::Text("%d", (int)md->meshes.size());
					}
				}
				column = 0;
				ImGui::TableNextRow();
				if( ImGui::TableSetColumnIndex(column++) )
					ImGui::Text("#textures");
				for( Model* md : models )
				{
					if( ImGui::TableSetColumnIndex(column++) )
					{
						if( md->meshes.size()==0 )
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
		// if( MapBaker::bakedNormalMapPointer != nullptr )
		// {
		// 	if( ImGui::Begin("Baked Normal Map##simp") )
		// 	{
		// 		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		// 		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		// 		glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
		// 		// ImGui::Text("%f %f", rectSize.x, rectSize.y);
		// 		const float minLength = glm::min(rectSize.x, rectSize.y);
		// 		GLuint rtId = MapBaker::bakedNormalMapPointer->getRenderedTex();
		// 		ImGui::Image(INT2VOIDP(rtId), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
		// 	}
		// 	ImGui::End();
		// }

		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data)
											{
			data->DesiredSize.x = glm::max(data->DesiredSize.x, data->DesiredSize.y);
			data->DesiredSize.y = data->DesiredSize.x; });

		if( ImGui::Begin("shadowMap##simp") && light.shadow_enabled )
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
		if( (GLFW_MOD_CONTROL == mods) && (GLFW_KEY_S == key) )
		{
			simplify_trigger = true;
			bake_trigger = true;
		}
		if( (GLFW_MOD_CONTROL == mods) && (GLFW_KEY_E == key) )
		{
			doExportModel(3, last_focused_vp_idx);
		}
		if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
	void AppSimplification::mouseBtnCallback(int button, int action, int mods)
	{
		// 이전에 선택된 viewport 저장
		if( viewports[last_focused_vp_idx]->hovered == false )
		{
			for( int i = 0; i < nr_viewports; i++ )
			{
				if( viewports[i]->hovered ) {
					if( last_focused_vp_idx != i ) {
						if( is_same_camera ) {
							viewports[last_focused_vp_idx]->camera.copySettingTo(viewports[i]->camera);
						}
						last_focused_vp_idx = i;
					}
				}
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
		for( int i = 0; i < nr_viewports; i++ )
		{
			if( viewports[i]->hovered )
			{
				selectedVpIdx = i;
			}
		}
		if( selectedVpIdx >= 0 )
		{
			doLoadModel(paths[0], selectedVpIdx);
		}
		else
		{
			log::err("you must drop to viewport\n");
		}
	}
}