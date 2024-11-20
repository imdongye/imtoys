/*

모델을 교체할때 scene.models[1]도 수정해줘야함

*/

#include "app_simplification.h"
#include "simplify.h"
#include <stb_image.h>
#include <stb_sprintf.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/save_file.h>
#include <limbrary/tools/text.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/3d/scene.h>
#include <limbrary/3d/model_io_helper.h>

#include <limbrary/using_in_cpp/std.h>
using namespace lim;

namespace
{
	vector<string> recent_model_paths;
}


AppSimplification::AppSimplification() : AppBase(1200, 780, APP_NAME, false)
{
	save_file::init();
	recent_model_paths.clear();
	if( save_file::data.empty() == false ) {
		recent_model_paths = save_file::data["recentModelPaths"];
	}


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

	ground.addOwn(new Texture());
	ground.own_textures.back()->initFromFile("assets/images/uv_grid.jpg", true);
	ground.root.mat = ground.addOwn(new Material());
	ground.own_materials.back()->BaseColor = {0,1,0};
	ground.own_materials.back()->map_ColorBase = ground.own_textures.back().raw;
	ground.own_materials.back()->map_Flags |= Material::MF_COLOR_BASE;
	ground.root.ms = ground.addOwn(new MeshPlane());
	ground.own_meshes.back()->initGL();
	ground.root.tf.pos = {0, 0, 0};
	ground.root.tf.scale = {10, 1, 10};
	ground.root.tf.update();
	ground.root.updateGlobalTransform();

	setProg(0);

	addEmptyViewport();
	doImportModel("assets/models/dwarf/Dwarf_2_Low.obj", 0);
	addEmptyViewport();

	key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
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
	};

	mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
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
	};
	dnd_callbacks[this] = [this](int count, const char **paths) {
		addEmptyViewport();
		doImportModel(paths[0], nr_viewports-1);
	};
}

AppSimplification::~AppSimplification()
{
	save_file::data["recentModelPaths"] = recent_model_paths;
	save_file::deinit();
	for( auto prog : programs )	delete prog;
	for( auto vp : viewports ) delete vp;
	for( auto scn : scenes ) delete scn;
}
void AppSimplification::addEmptyViewport()
{
	auto vp = new ViewportWithCam(new FramebufferMs, fmtStrToBuf("Viewport%d", nr_viewports));
	vp->camera.moveShift({0,1,-1.6f});
	vp->camera.updateViewMtx();
	viewports.push_back(vp);

	Scene* scn = new Scene();
	scn->addOwn(new ModelData());
	scn->addOwn(new ModelView(ground));
	scn->addOwn(new LightDirectional())->setShadowEnabled(true);

	scenes.push_back(scn);
	nr_viewports++;
}
void AppSimplification::subViewport(int vpIdx)
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
	delete scenes[vpIdx];
	scenes.erase(scenes.begin()+vpIdx);
	for( int i=vpIdx; i<nr_viewports; i++ ) {
		viewports[i]->name = fmtStrToBuf("viewport%d##simp", i);
	}
}



static void saveRecentModelPath(const char* path)
{
	// 절대경로를 상대경로로
	std::filesystem::path ap(path);
	std::string rp = std::filesystem::relative(ap, std::filesystem::current_path()).u8string();
	std::replace(rp.begin(), rp.end(), '\\', '/');
	log::pure("%s\n", rp.c_str());

	if( rp.empty() )
		return;

	auto samePathPos = std::find(recent_model_paths.begin(), recent_model_paths.end(), rp);
	if( samePathPos!=recent_model_paths.end() )
		recent_model_paths.erase(samePathPos);
	recent_model_paths.emplace_back(rp);
}



void AppSimplification::doImportModel(const char* path, int vpIdx)
{
	if( vpIdx<0||vpIdx>=nr_viewports ) {
		log::err("wrong vpIdx in load model");
		return;
	}

	ModelData& md = *(ModelData*)scenes[vpIdx]->own_mds[0].raw;

	if( !md.importFromFile(path, false, true) ) {
		return;
	}

	auto& vp = viewports[vpIdx];
	vp->camera.pivot.y = 1.f;
	vp->camera.pos.y = 1.5f;
	vp->camera.updateViewMtx();

	saveRecentModelPath(path);
}
void AppSimplification::doExportModel(size_t pIndex, int vpIdx)
{
	ModelData& md = *(ModelData*)scenes[vpIdx]->own_mds[0].raw;
	if( md.own_meshes.empty() ) {
		log::err("export\n");
		return;
	}
	md.exportToFile(pIndex, export_path);
}
void AppSimplification::doSimplifyModel(float lived_pct, int version, int agressiveness, bool verbose)
{
	ModelData& srcMd = *(ModelData*)scenes[src_vp_idx]->own_mds[0].raw;
	ModelData& dstMd = *(ModelData*)scenes[dst_vp_idx]->own_mds[0].raw;

	dstMd = srcMd; // copy

	simplifyModel(dstMd, lived_pct, version, agressiveness, verbose);
	int pct = 100.0 * dstMd.total_verts / srcMd.total_verts;
	dstMd.name += "_"+std::to_string(pct)+"_pct";

	viewports[dst_vp_idx]->camera.pivot = dstMd.root.tf.pos;
	viewports[dst_vp_idx]->camera.updateViewMtx();
}
void AppSimplification::doBakeNormalMap(int texSize)
{
	ModelData& srcMd = *(ModelData*)scenes[src_vp_idx]->own_mds[0].raw;
	ModelData& dstMd = *(ModelData*)scenes[dst_vp_idx]->own_mds[0].raw;
	if( srcMd.own_meshes.empty() || dstMd.own_meshes.empty() )
		log::err("You Must to simplify before baking\n");
	else
		bakeNormalMap( srcMd, dstMd, texSize );
}
void AppSimplification::setProg(int idx) {
	for(Scene* scn: scenes) {
		ModelData& md = *(ModelData*)scn->own_mds[0].raw;
		md.setProgToAllMat(programs[idx]);
	}
	ground.setProgToAllMat(programs[idx]);
}






void AppSimplification::update()
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

			ModelData& md = *(ModelData*)scenes[i]->own_mds[0].raw;

			md.root.dfsRender([&](const Mesh* ms, const Material* mat, const glm::mat4& _) {
				if( curMat != mat ) {
					curMat = mat;
					curMat->setUniformTo(prog);
				}
				if( curMesh != ms ) {
					curMesh = ms;
					curMesh->bindGL();
				}
				curMesh->bindAndDrawGL();
				return true;
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
			cam.updateProjMtx();
			scenes[i]->render(viewports[i]->getFb(), cam);
		}
		cam.aspect = backupAspect;
		cam.updateProjMtx();
	}
	else
	{
		for( int i=0; i<nr_viewports; i++ )
			scenes[i]->render(viewports[i]->getFb(), viewports[i]->camera);
	}
}
void AppSimplification::updateImGui()
{
	ModelData& srcMd = *(ModelData*)scenes[src_vp_idx]->own_mds[0].raw;
	ModelData& dstMd = *(ModelData*)scenes[dst_vp_idx]->own_mds[0].raw;

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
						for( iter = recent_model_paths.rbegin();
								iter != recent_model_paths.rend(); iter++ )
						{
							if( ImGui::MenuItem((*iter).c_str()) )
							{
								doImportModel((*iter).c_str(), i);
								break;
							}
						}
						if(ImGui::MenuItem("Clear Recent") )
						{
							recent_model_paths.clear();
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
					const ModelData& md = *(ModelData*)scenes[i]->own_mds[0].raw;
					if( md.own_meshes.empty() )
						continue;

					if( ImGui::BeginMenu(("Viewport" + std::to_string(i)).c_str()) )
					{
						int nr_formats = getNrExportFormats();
						for( int pIndex = 0; pIndex < nr_formats; pIndex++ )
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
		viewports[i]->drawImGuiAndUpdateCam();
		if( viewports[i]->is_opened == false ) {
			subViewport(i);
		}
	}

	if( ImGui::Begin("Simplify Options##simp") )
	{
		ImGui::Text("From viewport:");
		for( int i=0; i<nr_viewports; i++ )
		{
			ModelData& md = *(ModelData*)scenes[i]->own_mds[0].raw;
			if( md.own_meshes.empty() )
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
			log::pure("Done! %d => %d in %.3f sec. \n\n", srcMd.total_verts, dstMd.total_verts, simpTime);
		}

		ImGui::SameLine();
		ImGui::Text("target triangles = %d", (int)(srcMd.total_tris * pct));
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
			ModelData& md = *(ModelData*)scenes[src_vp_idx]->own_mds[0].raw;
			convertBumpMapToNormalMap(md);
		}
	}
	ImGui::End();

	if( ImGui::Begin("Viewing Options##simp") )
	{
		ImGui::Text("<camera>");
		ImGui::Checkbox("use same camera", &is_same_camera);
		ImGui::Text("camera fov: %f", viewports[last_focused_vp_idx]->camera.fovy);
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 3.0f));

		ImGui::Text("<shader>");
		LightDirectional& light = *scenes[dst_vp_idx]->own_dir_lits.back();
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
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				for( auto& mat : md.own_materials ) {
					mat->BumpHeight = bumpHeight;
				}
			}
		}
		static float texDelta = 0.00001f;
		if( ImGui::SliderFloat("texDelta", &texDelta, 0.000001f, 0.0001f, "%f") ) {
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				for( auto& mat : md.own_materials ) {
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
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%s", md.name.c_str());
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#verticies");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%d", md.total_verts);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#triangles");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%d", md.total_tris);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("boundary_x");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%f", md.boundary_size.x);
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("boundary_y");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%f", md.boundary_size.y);
				}
			}

			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#meshes");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%d", (int)md.own_meshes.size());
				}
			}
			column = 0;
			ImGui::TableNextRow();
			if( ImGui::TableSetColumnIndex(column++) )
				ImGui::Text("#textures");
			for( Scene* scn : scenes ) {
				ModelData& md = *(ModelData*)scn->own_mds[0].raw;
				if( ImGui::TableSetColumnIndex(column++) )
				{
					if( md.own_meshes.empty() )
						ImGui::Text("");
					else
						ImGui::Text("%d", (int)md.own_textures.size());
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
	LightDirectional& light = *scenes[dst_vp_idx]->own_dir_lits.back();
	if( ImGui::Begin("shadowMap##simp") && light.shadow->Enabled )
	{
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		glm::vec2 rectSize{vMax.x - vMin.x, vMax.y - vMin.y};
		// ImGui::Text("%f %f", rectSize.x, rectSize.y);
		const float minLength = glm::min(rectSize.x, rectSize.y);
		ImGui::Image(texIdToIg(light.shadow->map.getRenderedTexId()), ImVec2{minLength, minLength}, ImVec2{0, 1}, ImVec2{1, 0});
	}
	ImGui::End();
	ImGui::PopStyleVar();
}