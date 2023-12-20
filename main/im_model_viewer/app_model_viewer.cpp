/*
Todo:
1. Assimp 로딩속도 느림
2. 투명도 GL_BLEND적용안됨


*/


#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/model_io_helper.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <glad/glad.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	struct BrdfTestInfo {
		std::string ctrl_name = "##modelview";
		bool is_rotate_md = false;
		bool is_draw_floor = false;
		lim::Model* temp_floor_md = nullptr;
		int idx_Brdf = 2;
		int idx_D = 1;
		int idx_G = 0;
		int idx_F = 0;
		int idx_LitMod = 0;
		int nr_ibl_w_samples = 30;
        float shininess = 100.f; // shininess
        float refIdx    = 1.45f; // index of refraction
        float roughness = 0.3f;  // brdf param
		float metalness = 0.f;
		float ambientInt = 0.03f;
	};
	std::vector<BrdfTestInfo> brdf_test_infos;

	function<void(const Program&)> makeSetProg(BrdfTestInfo tInfo) {
		return [tInfo](const Program& prog) {
			prog.setUniform("idx_Brdf", tInfo.idx_Brdf);
			prog.setUniform("idx_D", tInfo.idx_D);
			prog.setUniform("idx_G", tInfo.idx_G);
			prog.setUniform("idx_F", tInfo.idx_F);
			prog.setUniform("idx_LitMod", tInfo.idx_LitMod);
			prog.setUniform("nr_ibl_w_samples", tInfo.nr_ibl_w_samples);
		};
	}

	int selected_vp_idx = 0;
}




lim::AppModelViewer::AppModelViewer() : AppBase(1373, 780, APP_NAME, false)
{
	viewports.reserve(5);
	scenes.reserve(5);
	
	program.name = "brdf_prog";
	program.attatch("mvp.vs").attatch("im_model_viewer/shaders/brdf.fs").link();

	light.setRotate(35.f, -35.f, 7.f);
	light.intensity = 55.f;

	light_model.name = "light";
	light_model.my_meshes.push_back(new MeshSphere(8, 4));
	light_model.root.addMeshWithMat(light_model.my_meshes.back()); // delete sphere when delete model!
	light_model.position = light.position;
	light_model.scale = glm::vec3(0.3f);
	light_model.updateModelMat();

	irr_prog.name = "irr baker";
	irr_prog.attatch("canvas.vs").attatch("im_model_viewer/shaders/irr_map.fs").link();
	setIBL("assets/ibls/artist_workshop_4k.hdr");

	
	AssetLib::get().default_material.prog = &program;

	addModelViewer("assets/models/objs/bunny.obj");
	addModelViewer("assets/models/objs/sphere20.obj");
	addModelViewer("assets/models/helmet/FlightHelmet/FlightHelmet.gltf");
}

lim::AppModelViewer::~AppModelViewer()
{
	brdf_test_infos.clear();
}

void lim::AppModelViewer::addModelViewer(string path) 
{
	Model* md = new Model();
	if( md->importFromFile(findModelInDirectory(path), true)==false )
		return;

	brdf_test_infos.push_back({});
	brdf_test_infos.back().ctrl_name = md->name+" ctrl ##modelviewer";

	md->my_materials.push_back(new Material());
	md->default_material = md->my_materials.back();
	md->default_material->prog = &program;
	md->default_material->set_prog = makeSetProg(brdf_test_infos.back());

	Model* floor = new Model("floor");
	floor->my_meshes.push_back(new MeshPlane(1));
	floor->root.addMeshWithMat(floor->my_meshes.back());
	floor->scale = glm::vec3(5.f);
	floor->position = {0,-md->pivoted_scaled_bottom_height,0};
	floor->updateModelMat();
	floor->my_materials.push_back(new Material());
	floor->default_material = floor->my_materials.back();
	floor->default_material->prog = &program;
	floor->default_material->set_prog = makeSetProg(brdf_test_infos.back());
	brdf_test_infos.back().temp_floor_md = floor;

	Scene scn;
	scn.lights.push_back(&light);
	scn.models.push_back(&light_model);
	scn.addOwnModel(md);
	scn.addOwnModel(floor);
	scn.models.pop_back();
	scn.map_Light = &light_map;
	scn.map_Irradiance = &irradiance_fb.color_tex;
	scenes.emplace_back(std::move(scn)); // vector move template error

	char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
	IFramebuffer* fb = new FramebufferMs(glm::max(utils::getMsMaxSamples(), 16), 4, 8);
	// IFramebuffer* fb = new FramebufferTexDepth(4, 8);
	// fb->blendable = true;
	viewports.emplace_back(vpName, fb);
	viewports.back().camera.setViewMode(CameraManVp::VM_PIVOT);
}
void lim::AppModelViewer::rmModelViewer(int idx)
{
	scenes.erase(scenes.begin()+idx);
	viewports.erase(viewports.begin()+idx);
	brdf_test_infos.erase(brdf_test_infos.begin()+idx);
}

void lim::AppModelViewer::drawModelsToViewports()
{
	for(int i=0; i<viewports.size(); i++ )
	{
		ViewportWithCamera& vp = viewports[i];
		if(selected_vp_idx!=i&&vp.is_focused)
			selected_vp_idx = i;
			
		render(vp.getFb(), vp.camera, scenes[i]);

		vp.getFb().bind();
		drawTexToQuad(irradiance_fb.getRenderedTex(),1.f);
		drawTexToQuad(light_map.tex_id, 1.f);
		vp.getFb().unbind();

		if( !vp.is_opened ) {
			rmModelViewer(i);
			i--;
			continue;
		}
	}
}


void lim::AppModelViewer::update() 	
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawModelsToViewports();
}
void lim::AppModelViewer::renderImGui()
{
	ImGui::DockSpaceOverViewport();

	// log::drawViewer("logger##model_viewer");

	// draw common ctrl
	{
		ImGui::Begin("common ctrl##model_viewer");
		static float litTheta = 90.f-glm::degrees(glm::acos(glm::dot(glm::normalize(light.position), glm::normalize(vec3(light.position.x, 0, light.position.z)))));
		const static float litThetaSpd = 70 * 0.001;
		static float litPhi = 90.f-glm::degrees(glm::atan(light.position.x,-light.position.z));
		const static float litPhiSpd = 360 * 0.001;
		static float litDist = glm::length(light.position);
		const static float litDistSpd = 45.f * 0.001;
		static bool isLightDraged = false;

		ImGui::Text("<light>");
		isLightDraged |= ImGui::DragFloat("yaw", &litPhi, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
		isLightDraged |= ImGui::DragFloat("pitch", &litTheta, litThetaSpd, 0, 80, "%.3f");
		isLightDraged |= ImGui::DragFloat("dist", &litDist, litDistSpd, 5.f, 50.f, "%.3f");
		if( isLightDraged ) {
			light.setRotate(litTheta, glm::fract(litPhi/360.f)*360.f, litDist);
			light_model.position = light.position;
			light_model.updateModelMat();
		}
		ImGui::Text("pos: %.1f %.1f %.1f", light.position.x, light.position.y, light.position.z);
		ImGui::SliderFloat("intencity", &light.intensity, 0.5f, 200.f, "%.1f");

		if( ImGui::Button("relead shader") ) {
			program.reload(GL_FRAGMENT_SHADER);
		}
		ImGui::End();
	}

	for(int i=0; i<scenes.size(); i++) 
	{
		// /draw model view
		viewports[i].drawImGui();

		// draw brdf ctrl
		if(selected_vp_idx != i)
			continue;
		Model& md = *scenes[i].my_mds[0]; 
		BrdfTestInfo& tInfo = brdf_test_infos[i];

		ImGui::Begin(tInfo.ctrl_name.c_str());
		ImGui::Checkbox("rotate", &tInfo.is_rotate_md);
		if( tInfo.is_rotate_md ) {
			md.orientation = glm::rotate(md.orientation, Q_PI*delta_time, {0,1,0});
			md.updateModelMat();
		}
		
		if( ImGui::Checkbox("floor", &tInfo.is_draw_floor) ) {
			if(tInfo.is_draw_floor) {
				scenes[i].models.push_back(tInfo.temp_floor_md);
			}
			else {
				scenes[i].models.pop_back();
			}
		}

		ImGui::Checkbox("draw envMap", &scenes[i].is_draw_env_map);
		ImGui::Separator();
		bool isInfoChanged = false;
		static const char* litModStrs[]={"point", "IBL(sampling)", "IBL(imp sampling)","IBL(pre-filtering)"};
		if( ImGui::Combo("Light", &tInfo.idx_LitMod, litModStrs, IM_ARRAYSIZE(litModStrs)) ) {
			isInfoChanged = true; 
		}
		if(tInfo.idx_LitMod==1||tInfo.idx_LitMod==2) {
			int nrSamples = tInfo.nr_ibl_w_samples*tInfo.nr_ibl_w_samples;
			if( ImGui::SliderInt("ibl samples", &nrSamples, 1, 2500, "%d") ) {
				tInfo.nr_ibl_w_samples = glm::sqrt(nrSamples);
				isInfoChanged = true; 
			}
			ImGui::Text("nr width samples %d", tInfo.nr_ibl_w_samples);
		}
		ImGui::Separator();
		static const char* modelStrs[]={"Phong", "BlinnPhong", "CookTorrance", "Oren-Nayar"};
		if( ImGui::Combo("Model", &tInfo.idx_Brdf, modelStrs, IM_ARRAYSIZE(modelStrs)) ) {
			isInfoChanged = true; 
		}
		if( tInfo.idx_Brdf==2 ) {
			static const char* dStrs[]={"BlinnPhong", "GGX", "Beckmann"};
			if( ImGui::Combo("D", &tInfo.idx_D, dStrs, IM_ARRAYSIZE(dStrs)) ) { isInfoChanged = true; }
			static const char* gStrs[]={"CookTorrance", "Smith"};
			if( ImGui::Combo("G", &tInfo.idx_G, gStrs, IM_ARRAYSIZE(gStrs)) ) { isInfoChanged = true; }
			static const char* fStrs[]={"SchlickNDV", "SchlickNDH", "SchlickHDV"};
			if( ImGui::Combo("F", &tInfo.idx_F, fStrs, IM_ARRAYSIZE(fStrs)) ) { isInfoChanged = true; }
		}
		if( isInfoChanged ) {
			md.default_material->set_prog = makeSetProg(tInfo);
		}
		if( tInfo.idx_Brdf<2 ) { // phong, blinn phong
			if( ImGui::SliderFloat("shininess", &tInfo.shininess, 0.5, 300) ) {
				for(Material* mat : md.my_materials) {
					mat->shininess = tInfo.shininess;
				}
			}
		}
		else { // cook-torrance
			if( ImGui::SliderFloat("roughness", &tInfo.roughness, 0.01, 1) ) {
				for(Material* mat : md.my_materials) {
					mat->roughness = tInfo.roughness;
				}
			}
			if( ImGui::SliderFloat("metalness", &tInfo.metalness, 0.000, 1) ) {
				for(Material* mat : md.my_materials) {
					mat->metalness = tInfo.metalness;
				}
			}
		}
		if( ImGui::SliderFloat("ambient light", &tInfo.ambientInt, 0.000, 0.05) ) {
			for(Material* mat : md.my_materials) {
				mat->ambientColor = glm::vec3(1)*tInfo.ambientInt;
			}
		}
		ImGui::End();
	}
}
void lim::AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
{
	// glFinish후 호출돼서 여기서 프로그램 리로드 해도됨.
	if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
		program.reload(GL_FRAGMENT_SHADER);
	}
}
void lim::AppModelViewer::cursorPosCallback(double xPos, double yPos)
{
}

void lim::AppModelViewer::setIBL(const char* path) {
	light_map.initFromFile(path, false);

	irradiance_fb.color_tex.updateFormat(3,32,false, true);
	irradiance_fb.resize(256, 128);
	lim::log::pure(": map Irr format\n\n");

	irradiance_fb.bind();
	irr_prog.use();
	irr_prog.setTexture("map_Light", light_map.tex_id, 0);
	AssetLib::get().screen_quad.drawGL();
	irradiance_fb.unbind();
}
void lim::AppModelViewer::dndCallback(int count, const char **paths)
{
	for( int i=0; i<count; i++ ) {
		const char* path = paths[i];
		if(strIsSame(getExtension(path),"hdr")) {
			setIBL(path);
		}
		else {
			addModelViewer(path);
		}
	}
}