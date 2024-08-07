//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <imgui.h>
#include <limbrary/asset_lib.h>
#include <limbrary/app_prefs.h>
#include <limbrary/log.h>
#include <limbrary/viewport.h>

#include "im_tests/app_template.h"
#include "im_tests/app_imgui_test.h"
#include "im_tests/app_icp.h"
#include "im_tests/app_gen_mesh.h"
#include "im_tests/app_astar.h"
#include "im_tests/app_moving_window.h"
#include "im_simplification/app_simplification.h"
#include "im_ray/app_ray.h"
#include "im_npr/app_hatching.h"
#include "im_hdr/app_hdr.h"
#include "im_anims/app_cloth_cpu.h"
#include "im_anims/app_fluid.h"
#include "im_shadertoy/app_shadertoy.h"
#include "im_sdf_modeler/app_sdf_modeler.h"
#include "im_mine_sweeper/app_mine_sweeper.h"
#include "im_model_viewer/app_model_viewer.h"
#include "im_anims/app_skeletal.h"
#include "im_anims/app_bvh_parsor.h"
#include "im_anims/app_cloth_gpu.h"
#include "im_anims/app_ik.h"
#include "im_anims/app_particle.h"
#include "im_pbd/app_pbd_cpu.h"
#include "im_pbd/app_pbd_gpu.h"
#include "im_anims/app_curve.h"


static int _selected_app_idx;

static std::vector<std::function<lim::AppBase*()>> _app_constructors;
static std::vector<const char*> _app_names;
static std::vector<const char*> _app_descriptions;

static void drawAppSellector()
{
	ImGuiIO io = ImGui::GetIO();
	const lim::AppBase& app = *lim::AssetLib::get().app;
	// draw app selector
	static bool isSelectorOpened = false;
	if( ImGui::IsKeyPressed(ImGuiKey_F1, false) ) {
		if( !isSelectorOpened ) {
			isSelectorOpened = true;
			ImGui::OpenPopup("AppSelector");
		}
		else {
			isSelectorOpened = false;
		}
	}
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if( ImGui::BeginPopupModal("AppSelector", &isSelectorOpened, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize) ) {
		for( int i = 0; i<_app_names.size(); i++ ) {
			if( ImGui::Button(_app_names[i]) ) {
				_selected_app_idx = i;
				isSelectorOpened = false;
				ImGui::CloseCurrentPopup();
				glfwSetWindowShouldClose(app.window, true);
			}
		}
		ImGui::EndPopup();
	}	
}



template <typename TApp>
static void pushAppData()
{
	_app_names.push_back(TApp::APP_NAME);
	_app_descriptions.push_back( TApp::APP_DESCRIPTION );
	_app_constructors.push_back( [](){ return new TApp(); } );
}


int main()
{
	pushAppData<lim::AppPbdCpu>();
	pushAppData<lim::AppIK>();
	pushAppData<lim::AppBvhParsor>();
	pushAppData<lim::AppRay>();
	pushAppData<lim::AppClothCPU>();
	pushAppData<lim::AppPbdGpu>();
	pushAppData<lim::AppCurve>();
	pushAppData<lim::AppClothGPU>();
	pushAppData<lim::AppParticle>();
	pushAppData<lim::AppSkeletal>();
	pushAppData<lim::AppMineSweeper>();
	pushAppData<lim::AppModelViewer>();
	pushAppData<lim::AppSdfModeler>();
	pushAppData<lim::AppTemplate>();
	pushAppData<lim::AppImGuiTest>();
	pushAppData<lim::AppICP>();
	pushAppData<lim::AppGenMesh>();
	pushAppData<lim::AppAstar>();
	pushAppData<lim::AppMovingWindow>();
	pushAppData<lim::AppSimplification>();
	pushAppData<lim::AppHatching>();
	pushAppData<lim::AppHdr>();
	pushAppData<lim::AppFluid>();
	pushAppData<lim::AppShaderToy>();

	_selected_app_idx = 0;

	if( _app_names.size()>1 ) {
		lim::AppBase::draw_appselector = drawAppSellector;
	}

	while( _selected_app_idx>=0 ) {
		lim::AppBase* app = _app_constructors[_selected_app_idx]();
		_selected_app_idx = -1;

		app->run();

		delete app;
	}

	return 0;
}