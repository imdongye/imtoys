/*

모델을 교체할때 scene.models[1]도 수정해줘야함

*/

#include "app_simplification.h"
#include "simplify.h"
#include <stb_image.h>
#include <stb_sprintf.h>
#include <limbrary/app_prefs.h>
#include <limbrary/model_view/mesh_maked.h>
#include <imgui.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/model_view/model_io_helper.h>


lim::AppSimplification::AppSimplification() : AppBase(1200, 780, APP_NAME, false)
{
	{
		Program* prog;
		prog = new Program("Normal Dot View");
		prog->attatch("mvp.vs").attatch("ndv.fs").link();
		programs.push_back(prog);
		prog = new Program("Normal Dot Light");
		prog->attatch("mvp.vs").attatch("ndl.fs").link();
		programs.push_back(prog);

		prog = new Program("Auto Normal");
		prog->home_dir = APP_DIR;
		prog->attatch("assets/shaders/mvp.vs").attatch("bumped.fs").link();
		programs.push_back(prog);

		prog = new Program("Bitoper");
		prog->home_dir = APP_DIR;
		prog->attatch("assets/shaders/mvp.vs").attatch("bitoper.fs").link();
		programs.push_back(prog);

		prog = new Program("Shadowed");
		prog->home_dir = APP_DIR;
		prog->attatch("shadowed.vs").attatch("shadowed.fs").link();
		programs.push_back(prog);

		prog = new Program("Map View, my normal");
		prog->home_dir = APP_DIR;
		prog->attatch("uv_view.vs").attatch("debug.fs").link();
		programs.push_back(prog);

		for( const auto& prog : programs )
			shader_names.push_back(prog->name.c_str());
	}

	ground.own_textures.push_back(new Texture());
	ground.own_textures.back()->initFromFile("assets/images/uv_grid.jpg", true);
	ground.own_materials.push_back(new Material());
	ground.own_materials.back()->BaseColor = {0,1,0};
	ground.own_materials.back()->map_ColorBase = ground.own_textures.back();
	ground.own_materials.back()->map_Flags |= Material::MF_COLOR_BASE;
	ground.own_meshes.push_back(new MeshPlane());
	ground.root.addMsMat(ground.own_meshes.back(), ground.own_materials.back());
	ground.tf->pos = {0, 0, 0};
	ground.tf->scale = {10, 1, 10};
	ground.tf->update();

	light.setShadowEnabled(true);

	setProg(0);

	addEmptyViewport();
	doImportModel("assets/models/dwarf/Dwarf_2_Low.obj", 0);
	addEmptyViewport();
}
lim::AppSimplification::~AppSimplification()
{
	for( auto prog : programs )	delete prog;
	for( auto vp : viewports ) delete vp;
	for( auto scn : scenes ) delete scn;
	for( auto md : models ) delete md;
}
void lim::AppSimplification::addEmptyViewport()
{
	auto vp = new ViewportWithCamera(fmtStrToBuf("viewport%d##simp", nr_viewports), new FramebufferMs);
	vp->camera.moveShift({0,1,-1.6f});
	vp->camera.updateViewMat();
	viewports.push_back(vp);

	models.push_back(new Model());

	Scene* scn = new Scene();
	scn->addRef(&ground);
	scn->addRef(models.back());
	scn->addRef(&light);
	scenes.push_back(scn);
	nr_viewports++;
}
void lim::AppSimplification::subViewport(int vpIdx)
{
	if( vpIdx<0||vpIdx>=nr_viewports ) {
		log::err("wrong vpIdx for close\n");
		return;
	}
	if( nr_viewports<=2 ) {
		log::err("you must have two viewports\n");
		viewports[vpIdx]->is_opened = true;
		return;
	}
	nr_viewports--;
	src_vp_idx = 0;
	dst_vp_idx = 1;
	last_focused_vp_idx = 0;

	delete viewports[vpIdx];
	viewports.erase(viewports.begin()+vpIdx);
	delete models[vpIdx];
	models.erase(models.begin()+vpIdx);
	delete scenes[vpIdx];
	scenes.erase(scenes.begin()+vpIdx);
	for( int i=vpIdx; i<nr_viewports; i++ ) {
		viewports[i]->name = fmtStrToBuf("viewport%d##simp", i);
	}
}
void lim::AppSimplification::doImportModel(std::string_view path, int vpIdx)
{
	if( vpIdx<0||vpIdx>=nr_viewports ) {
		log::err("wrong vpIdx in load model");
		return;
	}

	Model& md = *models[vpIdx];

	if( !md.importFromFile(path.data()) ) {
		return;
	}

	md.setUnitScaleAndPivot();
	md.tf->pos = {0,md.pivoted_scaled_bottom_height, 0};
	md.tf->update();

	auto& vp = viewports[vpIdx];
	vp->camera.pivot = md.tf->pos;
	vp->camera.position.y = md.tf->pos.y;
	vp->camera.updateViewMat();

	AppPrefs::get().saveRecentModelPath(path.data());
}
void lim::AppSimplification::doExportModel(size_t pIndex, int vpIdx)
{
	if(  models[vpIdx] == nullptr ) {
		log::err("export\n");
		return;
	}
	models[vpIdx]->exportToFile(pIndex, export_path);
}
void lim::AppSimplification::doSimplifyModel(float lived_pct, int version, int agressiveness, bool verbose)
{
	Model* srcMd = models[src_vp_idx];
	Model* dstMd = models[dst_vp_idx];

	if( dstMd != nullptr )
		delete dstMd;
	dstMd->copyFrom(*srcMd);

	simplifyModel(*dstMd, lived_pct, version, agressiveness, verbose);
	int pct = 100.0 * dstMd->nr_vertices / srcMd->nr_vertices;
	dstMd->name += "_"+std::to_string(pct)+"_pct";

	viewports[dst_vp_idx]->camera.pivot = dstMd->tf->pos;
	viewports[dst_vp_idx]->camera.updateViewMat();
}
void lim::AppSimplification::doBakeNormalMap(int texSize)
{
	if( models[src_vp_idx] != nullptr && models[dst_vp_idx] != nullptr )
		bakeNormalMap( *models[src_vp_idx], *models[dst_vp_idx], texSize );
	else
		log::err("You Must to simplify before baking\n");
}
void lim::AppSimplification::setProg(int idx) {
	for(Model* md: models) {
		md->setProgToAllMat(programs[idx]);
	}
	ground.setProgToAllMat(programs[idx]);
}






void lim::AppSimplification::update()
{
	// clear backbuffer
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// uv_view일때
	if( selected_prog_idx==programs.size()-1 ) {
		for( int i=0; i<nr_viewports; i++ ) 
		{
			const IFramebuffer& fb = viewports[i]->getFb();
			const Program& prog = *programs[selected_prog_idx];
			const Material* curMat = nullptr;
			const Mesh* curMesh = nullptr;

			fb.bind();
			prog.use();

			models[i]->root.treversal([&](const Mesh* ms, const Material* mat, const glm::mat4& tf) {
				if( curMat != mat ) {
					curMat = mat;
					curMat->setUniformTo(prog);
				}
				if( curMesh != ms ) {
					curMesh = ms;
					curMesh->bindGL();
				}
				curMesh->bindAndDrawGL();
			});

			fb.unbind();
		}
		return;
	}

	if( is_same_camera )
	{
		Camera& cam = viewports[last_focused_vp_idx]->camera; 
		float backupAspect = cam.aspect;
		for( int i=0; i<nr_viewports; i++ ) {
			cam.aspect = (i==last_focused_vp_idx)? backupAspect : viewports[i]->camera.aspect;
			cam.updateProjMat();
			render(viewports[i]->getFb(), cam, *scenes[i]);
		}
		cam.aspect = backupAspect;
		cam.updateProjMat();
	}
	else
	{
		for( int i=0; i<nr_viewports; i++ )
			render(viewports[i]->getFb(), viewports[i]->camera, *scenes[i]);
	}
}
void lim::AppSimplification::updateImGui()
{
	const Model& srcMd = *models[src_vp_idx];
	const Model& dstMd = *models[dst_vp_idx];

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
						for( iter = AppPrefs::get().recent_model_paths.rbegin();
								iter != AppPrefs::get().recent_model_paths.rend(); iter++ )
						{
							if( ImGui::MenuItem((*iter).c_str()) )
							{
								doImportModel((*iter), i);
								break;
							}
						}
						if(ImGui::MenuItem("Clear Recent") )
						{
							AppPrefs::get().clearData();
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
					if( md.own_meshes.size()==0 )
						continue;

					if( ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str()) )
					{
						int nr_formats = getNrExportFormats();
						for( size_t pIndex = 0; pIndex < nr_formats; pIndex++ )
						{
							const aiExportFormatDesc* format = getExportFormatInfo(pIndex);
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
		if( viewports[i]->drawImGui() == false ) {
			subViewport(i);
		}
	}

	log::drawViewer("log viwer##simplification");

	if( ImGui::Begin("Simplify Options##simp") )
	{
		ImGui::Text("From viewport:");
		for( int i=0; i<nr_viewports; i++ )
		{
			if( models[i]->own_meshes.size() == 0 )
				continue;
			ImGui::SameLine();
			if( ImGui::RadioButton( fmtStrToBuf("%d##1", i), &src_vp_idx, i) && src_vp_idx == dst_vp_idx ) {
				dst_vp_idx++;
				dst_vp_idx %= nr_viewports;
			}
		}

		ImGui::Text("to viewport:");
		for( int i=0; i<nr_viewports; i++ )
		{
			if( i == src_vp_idx )
				continue;
			ImGui::SameLine();
			ImGui::RadioButton((std::to_string(i) + "##2").c_str(), &dst_vp_idx, i);
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
			log::pure("\nSimplifing %s..... . ... ... .. .. . .  .\n", srcMd.name.c_str());
			simpTime = glfwGetTime();
			simplify_trigger = false;
			doSimplifyModel((float)pct / 100.f, selectedVersion, agressiveness, verbose);
			simpTime = glfwGetTime()-simpTime;
			log::pure("Done! %d => %d in %.3f sec. \n\n", srcMd.nr_vertices, dstMd.nr_vertices, simpTime);
		}

		ImGui::SameLine();
		ImGui::Text("target triangles = %d", (int)(srcMd.nr_triangles * pct));
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
		if( ImGui::Button("BumpMap To Normal Map") )
		{
			convertBumpMapToNormalMap(*models[src_vp_idx]);
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
			viewports[last_focused_vp_idx]->camera.setViewMode((CameraController::VIEWING_MODE)viewMode);
		}
		ImGui::Checkbox("use same camera", &is_same_camera);
		static float cameraMoveSpeed = 4.0f;
		if(ImGui::SliderFloat("move speed", &cameraMoveSpeed, 2.0f, 6.0f)) {
			viewports[last_focused_vp_idx]->camera.spd_free_move = cameraMoveSpeed;
		}
		ImGui::Text("camera fov: %f", viewports[last_focused_vp_idx]->camera.fovy);
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 3.0f));

		ImGui::Text("<shader>");
		if( ImGui::Combo("type", &selected_prog_idx, shader_names.data(), shader_names.size()) )
		{
			setProg(selected_prog_idx);
			light.setShadowEnabled(strcmp(shader_names[selected_prog_idx], "Shadowed")==0);
		}
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 3.0f));

		ImGui::Text("<light>");
		const static float litThetaSpd = 70 * 0.001;
		const static float litPhiSpd = 360 * 0.001;
		static bool isLightDraged = false;
		isLightDraged |= ImGui::DragFloat("light phi", &light.tf.phi, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
		isLightDraged |= ImGui::DragFloat("light theta", &light.tf.theta, litThetaSpd, 0, 80, "%.3f");
		if( isLightDraged ) {
			light.tf.updateWithRotAndDist();
		}
		ImGui::Text("pos %.1f %.1f %.1f", light.tf.pos.x, light.tf.pos.y, light.tf.pos.z);

		static float bumpHeight = 100;
		if( ImGui::SliderFloat("bumpHeight", &bumpHeight, 0.0, 300.0) ) {
			for( Model* md : models ) {
				for( Material* mat : md->own_materials ) {
					mat->BumpHeight = bumpHeight;
				}
			}
		}
		static float texDelta = 0.00001;
		if( ImGui::SliderFloat("texDelta", &texDelta, 0.000001, 0.0001, "%f") ) {
			for( Model* md : models ) {
				for( Material* mat : md->own_materials ) {
					mat->TexDelta = texDelta;
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
			for( const Viewport* vp : viewports )
			{
				ImGui::TableSetupColumn(vp->name.c_str());
			}
			ImGui::TableHeadersRow();

			int column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("name");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%s", md->name.c_str());
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#verticies");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%d", md->nr_vertices);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#triangles");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%d", md->nr_triangles);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("boundary_x");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%f", md->boundary_size.x);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("boundary_y");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%f", md->boundary_size.y);
				}
			}

			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#meshes");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%d", (int)md->own_meshes.size());
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#textures");
			for( const Model* md : models )
			{
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md->own_meshes.size()==0 )
						ImGui::Text("");
					else
						ImGui::Text("%d", (int)md->own_textures.size());
				}
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();

	/* show texture */
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});


	// Todo
	// ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
	// 	data->DesiredSize.x = glm::max(data->DesiredSize.x, data->DesiredSize.y);
	// 	data->DesiredSize.y = data->DesiredSize.x; 
	// });
	// if( MapBaker::bakedNormalMapPointer != nullptr )
	// {
	// 	if( ImGui::Begin("Baked Normal Map##simp") )
	// 	{
	// 		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	// 		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
	// 		glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
	// 		// ImGui::Text("%f %f", rectSize.x, rectSize.y);
	// 		const float minLength = glm::min(rectSize.x, rectSize.y);
	// 		GLuint rtId = MapBaker::bakedNormalMapPointer.getRenderedTexId();
	// 		ImGui::Image(INT2VOIDP(rtId), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
	// 	}
	// 	ImGui::End();
	// }

	ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
		data->DesiredSize.x = glm::max(data->DesiredSize.x, data->DesiredSize.y);
		data->DesiredSize.y = data->DesiredSize.x; 
	});

	if( ImGui::Begin("shadowMap##simp") && light.shadow->Enabled )
	{
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
		// ImGui::Text("%f %f", rectSize.x, rectSize.y);
		const float minLength = glm::min(rectSize.x, rectSize.y);
		ImGui::Image(INT2VOIDP(light.shadow->map.getRenderedTexId()), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void lim::AppSimplification::keyCallback(int key, int scancode, int action, int mods)
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
void lim::AppSimplification::mouseBtnCallback(int button, int action, int mods)
{
	// 이전에 선택된 viewport 저장
	if( viewports[last_focused_vp_idx]->is_hovered == false )
	{
		for( int i = 0; i < nr_viewports; i++ )
		{
			if( last_focused_vp_idx == i )
				continue;
			// 새로운 viewport를 선택하면
			if( viewports[i]->is_hovered ) {
				if( is_same_camera ) {
					viewports[i]->camera.copyFrom(viewports[last_focused_vp_idx]->camera);
					viewports[i]->camera.viewing_mode = viewports[last_focused_vp_idx]->camera.viewing_mode;
				}
				last_focused_vp_idx = i;
			}
		}
	}
}
void lim::AppSimplification::dndCallback(int count, const char **paths)
{
	addEmptyViewport();
	doImportModel(paths[0], nr_viewports-1);
}