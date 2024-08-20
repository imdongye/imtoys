/*
Todo:
1. Assimp 로딩속도 느림 -> 릴리즈 빌드
2. 투명도 GL_BLEND적용안됨
3. prefilter 버튼 빼기

*/


#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/model_io_helper.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/limgui.h>
#include <limbrary/glm_tools.h>
#include <glad/glad.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	struct BrdfTestInfo {
		std::string ctrl_name = "##modelview";
		bool is_rotate_md = false;
		bool is_draw_floor = true;
		int idx_Brdf = 2;
		int idx_D = 1;
		int idx_G = 0;
		int idx_F = 0;
		int idx_LitMod = 0;
		int nr_IblWSamples = 30;
        float shininess = 100.f; // shininess
        // float refIdx    = 1.45f; // index of refraction
        float roughness = 0.3f;  // brdf param
		float metalness = 0.f;
		float ambient_int = 0.008f;
		void setup(Model& md) {
			for(Material* mat : md.own_materials) {
				mat->Shininess = shininess;
			}
			for(Material* mat : md.own_materials) {
				mat->Roughness = roughness;
			}
			for(Material* mat : md.own_materials) {
				mat->Metalness = metalness;
			}
			for(Material* mat : md.own_materials) {
				mat->AmbientColor = glm::vec3(1)*ambient_int;
			}
		}
	};
	std::vector<BrdfTestInfo> brdf_test_infos;

	function<void(const Program&)> makeSetProg(BrdfTestInfo tInfo) {
		return [tInfo](const Program& prog) {
			prog.setUniform("idx_Brdf", tInfo.idx_Brdf);
			prog.setUniform("idx_D", tInfo.idx_D);
			prog.setUniform("idx_G", tInfo.idx_G);
			prog.setUniform("idx_F", tInfo.idx_F);
			prog.setUniform("idx_LitMod", tInfo.idx_LitMod);
			prog.setUniform("nr_IblWSamples", tInfo.nr_IblWSamples);
		};
	}

	int selected_vp_idx = 0;
}




lim::AppModelViewer::AppModelViewer() : AppBase(1373, 780, APP_NAME, false)
	, vp_light_map("light map", new FramebufferNoDepth(3,32))
	, vp_irr_map("irr map", new FramebufferNoDepth(3,32))
	, vp_pfenv_map("pf env map", new FramebufferNoDepth(3,32))
	, vp_pfbrdf_map("pf brdf map", new FramebufferNoDepth(3,32))
	, ib_light("assets/images/ibls/artist_workshop_4k.hdr")
{
	viewports.reserve(5);
	scenes.reserve(5);
	
	program.name = "brdf_prog";
	program.attatch("mvp_shadow.vs").attatch("im_model_viewer/shaders/brdf.fs").link();

	floor_md.name = "floor";
	floor_md.addOwn(new MeshPlane(2.f, 2.f));
	floor_md.own_meshes.back()->initGL(true);
	Material* flMat = floor_md.addOwn(new Material());
	flMat->prog = &program;
	floor_md.root.addMsMat(floor_md.own_meshes.back(), flMat);
	floor_md.root.tf.scale = glm::vec3(5.f);
	floor_md.root.tf.update();
	floor_md.root.updateGlobalTransform();

	Texture* flTex;
	flTex = floor_md.addOwn(new Texture());
	flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
	flTex->initFromFile("assets/images/floor/wood_floor_deck_diff_4k.jpg", true);
	flMat->map_Flags |= Material::MF_COLOR_BASE;
	flMat->map_ColorBase = flTex;

	flTex = floor_md.addOwn(new Texture());
	flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
	flTex->initFromFile("assets/images/floor/wood_floor_deck_arm_4k.jpg");
	flMat->map_Flags |= Material::MF_ARM;
	flMat->map_Roughness = flTex;

	flTex = floor_md.addOwn(new Texture());
	flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
	flTex->initFromFile("assets/images/floor/wood_floor_deck_nor_gl_4k.jpg");
	flMat->map_Flags |= Material::MF_NOR;
	flMat->map_Bump = flTex;

	addModelViewer("assets/models/objs/bunny.obj");
	// addModelViewer("assets/models/objs/sphere20.obj");
	// addModelViewer("assets/models/helmet/FlightHelmet/FlightHelmet.gltf");
}

lim::AppModelViewer::~AppModelViewer()
{
	brdf_test_infos.clear();
}

void lim::AppModelViewer::addModelViewer(string path) 
{
	Model* md = new Model();
	if( md->importFromFile(findModelInDirectory(path), false, true)==false ) {
		delete md;
		return;
	}
	LightDirectional* lit = new LightDirectional();
	lit->tf.theta = 35.f;
	lit->tf.phi =  -35.f;
	lit->tf.dist = 7.f;
	lit->tf.updateWithRotAndDist();
	lit->Intensity = 55.f;
	lit->setShadowEnabled(true);

	brdf_test_infos.push_back({});
	brdf_test_infos.back().ctrl_name = md->name+" ctrl ##modelviewer";
	brdf_test_infos.back().is_draw_floor = true;

	brdf_test_infos.back().setup(*md);

	md->setProgToAllMat(&program);
	md->setSetProgToAllMat(makeSetProg(brdf_test_infos.back()));

	Scene* scn = new Scene();
	scn->addOwn(lit);
	scn->addOwn(md);
	scn->addOwn(new ModelView(floor_md));
	scn->ib_light = &ib_light;
	scenes.push_back(scn);

	char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
	auto vp = new ViewportWithCamera(vpName, new FramebufferMs(8));
	vp->camera.setViewMode(CameraManVp::VM_PIVOT);
	vp->camera.moveShift({0,1.f,0});
	vp->camera.updateViewMat();
	viewports.push_back(vp);
}
void lim::AppModelViewer::rmModelViewer(int idx)
{
	delete scenes[idx];
	delete viewports[idx];
	scenes.erase(scenes.begin()+idx);
	viewports.erase(viewports.begin()+idx);
	brdf_test_infos.erase(brdf_test_infos.begin()+idx);
}

void lim::AppModelViewer::drawModelsToViewports()
{
	for(int i=0; i<viewports.size(); i++ )
	{
		ViewportWithCamera& vp = *viewports[i];
		if( selected_vp_idx!=i && vp.is_focused )
			selected_vp_idx = i;
			
		render(vp.getFb(), vp.camera, *scenes[i], true);

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
void lim::AppModelViewer::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	// log::drawViewer("logger##model_viewer");

	LimGui::LightDirectionalEditor(*(LightDirectional*)scenes[selected_vp_idx]->own_lits[0]);

	// draw common ctrl
	{
		ImGui::Begin("common ctrl##model_viewer");

		if( ImGui::Button("relead shader(ctrl+R)") ) {
			program.reload(GL_FRAGMENT_SHADER);
		}

		static bool is_draw_light_map_vp = false;
		static float pfenv_depth = 0.f;
		ImGui::Checkbox("show light map", &is_draw_light_map_vp);
		if( is_draw_light_map_vp ) {

			vp_light_map.getFb().bind();
			drawTexToQuad(ib_light.getTexIdLight(), 2.2f, 0.f, 1.f);
			vp_light_map.getFb().unbind();
			vp_light_map.drawImGui();

			ImGui::SliderFloat("pfenv depth", &pfenv_depth, 0.f, 1.f);
				
			vp_irr_map.getFb().bind();
			drawTexToQuad(ib_light.getTexIdIrradiance(), 2.2f, 0.f, 1.f);
			vp_irr_map.getFb().unbind();
			vp_irr_map.drawImGui();

			vp_pfenv_map.getFb().bind();
			drawTex3dToQuad(ib_light.getTexIdPreFilteredEnv(), pfenv_depth, 2.2f, 0.f, 1.f);
			vp_pfenv_map.getFb().unbind();
			vp_pfenv_map.drawImGui();

			vp_pfbrdf_map.getFb().bind();
			drawTexToQuad(ib_light.getTexIdPreFilteredBRDF(), 2.2f, 0.f, 1.f);
			vp_pfbrdf_map.getFb().unbind();
			vp_pfbrdf_map.drawImGui();
		}
		
		ImGui::End();
	}

	for(int i=0; i<scenes.size(); i++) 
	{
		// /draw model view
		viewports[i]->drawImGui();

		// draw brdf ctrl
		if(selected_vp_idx != i)
			continue;
		ModelView& md = *scenes[i]->own_mds[0]; 
		BrdfTestInfo& tInfo = brdf_test_infos[i];

		ImGui::Begin(tInfo.ctrl_name.c_str());
		ImGui::Checkbox("rotate", &tInfo.is_rotate_md);
		if( tInfo.is_rotate_md ) {
			md.root.tf.ori = glm::rotate(md.root.tf.ori, glim::pi45*delta_time, {0,1,0});
			md.root.tf.update();
			md.root.updateGlobalTransform();

		}
		
		if( ImGui::Checkbox("floor", &tInfo.is_draw_floor) ) {
			if(tInfo.is_draw_floor) {
				scenes[i]->own_mds[1]->root.enabled = true;
			}
			else {
				scenes[i]->own_mds[1]->root.enabled = false;
			}
		}

		ImGui::Checkbox("draw envMap", &scenes[i]->is_draw_env_map);
		ImGui::Separator();
		bool isInfoChanged = false;
		static const char* litModStrs[]={"point", "IBL(sampling)", "IBL(imp sampling)","IBL(pre-filtering)"};
		int nrLitMods = IM_ARRAYSIZE(litModStrs);
		if( ImGui::Combo("Light", &tInfo.idx_LitMod, litModStrs, nrLitMods) ) {
			isInfoChanged = true; 
		}
		if(tInfo.idx_LitMod==1||tInfo.idx_LitMod==2) {
			int nrSamples = tInfo.nr_IblWSamples*tInfo.nr_IblWSamples/2;
			if( ImGui::SliderInt("ibl samples", &nrSamples, 1, 2500, "%d") ) {
				tInfo.nr_IblWSamples = (int)glm::sqrt(2*nrSamples);
				isInfoChanged = true; 
			}
			ImGui::Text("nr width samples (%dx%d)", tInfo.nr_IblWSamples, tInfo.nr_IblWSamples/2);
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
			md.md_data->setSetProgToAllMat(makeSetProg(tInfo));
		}
		if( tInfo.idx_Brdf<2 ) { // phong, blinn phong
			if( ImGui::SliderFloat("shininess", &tInfo.shininess, 0.5f, 300) ) {
				for(Material* mat : md.md_data->own_materials) {
					mat->Shininess = tInfo.shininess;
				}
			}
		}
		else { // cook-torrance
			if( ImGui::SliderFloat("roughness", &tInfo.roughness, 0.015f, 1) ) {
				for(Material* mat : md.md_data->own_materials) {
					mat->Roughness = tInfo.roughness;
				}
			}
			if( ImGui::SliderFloat("metalness", &tInfo.metalness, 0.000f, 1) ) {
				for(Material* mat : md.md_data->own_materials) {
					mat->Metalness = tInfo.metalness;
				}
			}
		}
		if( ImGui::SliderFloat("ambient light", &tInfo.ambient_int, 0.000, 0.05f) ) {
			for(Material* mat : md.md_data->own_materials) {
				mat->AmbientColor = glm::vec3(1)*tInfo.ambient_int;
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
void lim::AppModelViewer::dndCallback(int count, const char **paths)
{
	for( int i=0; i<count; i++ ) {
		const char* path = paths[i];
		if(strIsSame(getExtension(path),"hdr")) {
			for(BrdfTestInfo& tInfo: brdf_test_infos) {
				tInfo.idx_LitMod = 0;
			}
			ib_light.setMapAndBake(path);
		}
		else {
			addModelViewer(path);
		}
	}
}