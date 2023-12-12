#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/model_io_helper.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	struct BrdfTestInfo {
		std::string ctrlName = "##modelview";
		bool isRotated = false;
		int idx_Brdf = 2;
		int idx_D = 1;
		int idx_G = 0;
		int idx_F = 0;
        float shininess = 100.f; // shininess
        float refIdx    = 1.45f; // index of refraction
        float roughness = 0.3f;  // brdf param
		float metalness = 0.f;
		float ambientInt = 0.03f;
	};
	std::vector<BrdfTestInfo> brdfTestInfos;

	function<void(const Program&)> makeSetProg(BrdfTestInfo tInfo) {
		return [tInfo](const Program& prog) {
			prog.setUniform("idx_Brdf", tInfo.idx_Brdf);
			prog.setUniform("idx_D", tInfo.idx_D);
			prog.setUniform("idx_G", tInfo.idx_G);
			prog.setUniform("idx_F", tInfo.idx_F);
		};
	}
}



namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(780, 780, APP_NAME, false)
	{
		viewports.reserve(5);
		scenes.reserve(5);
		
		program.name = "brdf_prog";
		program.home_dir = APP_DIR;
		program.attatch("mvp.vs").attatch("brdf.fs").link();

		light.setRotate(35.f, -35.f, 7.f);
		light.intensity = 55.f;

		light_model.name = "light";
		light_model.my_meshes.push_back(new MeshSphere(8, 4));
		light_model.root.addMeshWithMat(light_model.my_meshes.back()); // delete sphere when delete model!
		light_model.position = light.position;
		light_model.scale = glm::vec3(0.3f);
		light_model.updateModelMat();

		light_map.initFromFile("assets/ibls/artist_workshop_4k.hdr", false);

		AssetLib::get().default_material.prog = &program;

		addModelViewer("assets/models/objs/bunny.obj");

		GLint tempInt;
		glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &tempInt);
		log::pure("GL_MAX_INTEGER_SAMPLES : %d\n", tempInt);
		glGetIntegerv(GL_MAX_SAMPLES, &tempInt);
		log::pure("GL_MAX_SAMPLES : %d\n", tempInt);
	}
	AppModelViewer::~AppModelViewer()
	{
		brdfTestInfos.clear();
	}

	void AppModelViewer::addModelViewer(string path) 
	{
		Model* md = new Model();
		if( md->importFromFile(findModelInDirectory(path), true)==false )
			return;

		brdfTestInfos.push_back({});
		brdfTestInfos.back().ctrlName = md->name+" ctrl ##modelviewer";

		md->my_materials.push_back(new Material());
		md->default_material = md->my_materials.back();
		md->default_material->prog = &program;
		md->default_material->set_prog = makeSetProg(brdfTestInfos.back());

		Model* floor = new Model("floor");
		floor->my_meshes.push_back(new MeshPlane(1));
		floor->root.addMeshWithMat(floor->my_meshes.back());
		floor->scale = glm::vec3(5.f);
		floor->position = {0,-md->pivoted_scaled_bottom_height,0};
		floor->updateModelMat();
		floor->my_materials.push_back(new Material());
		floor->default_material = floor->my_materials.back();
		floor->default_material->prog = &program;
		floor->default_material->set_prog = makeSetProg(brdfTestInfos.back());

		Scene scn;
		scn.lights.push_back(&light);
		scn.models.push_back(&light_model);
		scn.addOwnModel(md);
		scn.addOwnModel(floor);
		scn.map_Light = &light_map;
		scenes.emplace_back(std::move(scn)); // vector move template error

		char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
		viewports.emplace_back(vpName, new FramebufferMs(4));
		viewports.back().camera.setViewMode(CameraManVp::VM_PIVOT);
	}
	void AppModelViewer::rmModelViewer(int idx)
	{
		scenes.erase(scenes.begin()+idx);
		viewports.erase(viewports.begin()+idx);
		brdfTestInfos.erase(brdfTestInfos.begin()+idx);
	}
	
	void AppModelViewer::drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ )
		{
			render(viewports[i].getFb(), viewports[i].camera, scenes[i]);

			if( !viewports[i].is_opened ) {
				rmModelViewer(i);
				i--;
				continue;
			}
		}
	}


	void AppModelViewer::update() 	
	{
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawModelsToViewports();
	}
	void AppModelViewer::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		log::drawViewer("logger##model_viewer");

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
			// draw model ctrl
			{
				Model& md = *scenes[i].my_mds[0]; 
				BrdfTestInfo& tInfo = brdfTestInfos[i];

				ImGui::Begin(tInfo.ctrlName.c_str());
				ImGui::Checkbox("rotate", &tInfo.isRotated);
				if( tInfo.isRotated ) {
					md.orientation = glm::rotate(md.orientation, Q_PI*delta_time, {0,1,0});
					md.updateModelMat();
				}
				bool isInfoChanged = false;
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
				if( ImGui::SliderFloat("ambient light", &tInfo.ambientInt, 0.000, 0.1) ) {
					for(Material* mat : md.my_materials) {
						mat->ambientColor = glm::vec3(1)*tInfo.ambientInt;
					}
				}
				ImGui::End();
			}

			// /draw model view
			viewports[i].drawImGui();
		}
	}
	void AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
	{
		// glFinish후 호출돼서 여기서 프로그램 리로드 해도됨.
		if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
			program.reload(GL_FRAGMENT_SHADER);
		}
	}
	void AppModelViewer::cursorPosCallback(double xPos, double yPos)
	{
	}
	void AppModelViewer::dndCallback(int count, const char **paths)
	{
		for( int i=0; i<count; i++ ) {
			const char* path = paths[i];
			addModelViewer(path);
		}
	}
}