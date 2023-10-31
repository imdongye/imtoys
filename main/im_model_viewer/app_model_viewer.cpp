#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	struct BrdfTestInfo {
		std::string ctrlName = "##modelview";
		int model_idx = 0;
		int D_idx = 0;
		int G_idx = 0;
		int F_idx = 0;
        float shininess = 100.f; // shininess
        float refIdx    = 1.45f; // index of refraction
        float roughness = 0.3f;  // brdf param
	};
	std::vector<BrdfTestInfo> brdfTestInfos;

	function<void(const Program&)> makeSetProg(BrdfTestInfo tInfo) {
		return [tInfo](const Program& prog) {
			prog.setUniform("model_idx", tInfo.model_idx);
			prog.setUniform("D_idx", tInfo.D_idx);
			prog.setUniform("G_idx", tInfo.G_idx);
			prog.setUniform("F_idx", tInfo.F_idx);
		};
	}
}



namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(780, 780, APP_NAME, false)
	{
		viewports.reserve(5);
		scenes.reserve(5);

		program.home_dir = APP_DIR;
		program.attatch("mvp.vs").attatch("brdf.fs").link();

		light.setRotate(35.f, -35.f, 5.f);
		light.intensity = 15.f;
		light_model.name = "light";
		light_model.my_meshes.push_back(new MeshSphere(8, 4));
		light_model.root.addMeshWithMat(light_model.my_meshes.back()); // delete sphere when delete model!
		light_model.position = light.position;
		light_model.scale = glm::vec3(0.3f);
		light_model.updateModelMat();

		AssetLib::get().default_material.prog = &program;

		addModelViewer("assets/models/objs/bunny.obj");
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

		md->my_materials.push_back(new Material());
		md->default_material = md->my_materials.back();
		md->default_material->prog = &program;

		brdfTestInfos.push_back({});
		brdfTestInfos.back().ctrlName = md->name+" ctrl ##modelviewer";
		md->default_material->set_prog = makeSetProg(brdfTestInfos.back());

		Scene scn;
		scn.lights.push_back(&light);
		scn.models.push_back(&light_model);
		scn.models.push_back(md);
		scn.my_mds.push_back(md); // 0번째
		scenes.emplace_back(std::move(scn));

		char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
		viewports.emplace_back(vpName, new FramebufferMs(8));
		viewports.back().camera.setViewMode(CameraManVp::VM_PIVOT);
	}
	void AppModelViewer::subModelViewer(int idx)
	{
		scenes.erase(scenes.begin()+idx);
		viewports.erase(viewports.begin()+idx);
		brdfTestInfos.erase(brdfTestInfos.begin()+idx);
	}
	
	void AppModelViewer::drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ ) {
			render(viewports[i].getFb(), viewports[i].camera, scenes[i]);
			if( !viewports[i].is_opened ) {
				subModelViewer(i);
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
			isLightDraged |= ImGui::DragFloat("pitch", &litTheta, litThetaSpd, 0, 80, "%.3f");
			isLightDraged |= ImGui::DragFloat("yaw", &litPhi, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
			isLightDraged |= ImGui::DragFloat("dist", &litDist, litDistSpd, 5.f, 50.f, "%.3f");
			if( isLightDraged ) {
				light.setRotate(litTheta, glm::fract(litPhi/360.f)*360.f, litDist);
				light_model.position = light.position;
				light_model.updateModelMat();
			}
			ImGui::Text("pos: %.1f %.1f %.1f", light.position.x, light.position.y, light.position.z);
			ImGui::SliderFloat("intencity", &light.intensity, 0.5f, 60.f, "%.1f");

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

				bool isInfoChanged = false;
				static const char* modelStrs[]={"Phong", "BlinnPhong", "CookTorrance"};

				if( ImGui::Combo("Model", &tInfo.model_idx, modelStrs, IM_ARRAYSIZE(modelStrs)) ) {
					isInfoChanged = true; 
				}
				if( tInfo.model_idx==2 ) {
					static const char* dStrs[]={"BlinnPhong", "GGX", "Beckmann"};
					if( ImGui::Combo("D", &tInfo.D_idx, dStrs, IM_ARRAYSIZE(dStrs)) ) { isInfoChanged = true; }
					static const char* gStrs[]={"CookTorrance"};
					if( ImGui::Combo("G", &tInfo.G_idx, gStrs, IM_ARRAYSIZE(gStrs)) ) { isInfoChanged = true; }
					static const char* fStrs[]={"Schlick"};
					if( ImGui::Combo("F", &tInfo.F_idx, fStrs, IM_ARRAYSIZE(fStrs)) ) { isInfoChanged = true; }
				}
				if( isInfoChanged ) {
					md.default_material->set_prog = makeSetProg(tInfo);
				}
				if( ImGui::SliderFloat("shininess", &tInfo.shininess, 0, 1000) ) {
					for(Material* mat : md.my_materials) {
						mat->shininess = tInfo.shininess;
					}
				}
				if( ImGui::SliderFloat("roughness", &tInfo.roughness, 0.001, 1) ) {
					for(Material* mat : md.my_materials) {
						mat->roughness = tInfo.roughness;
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