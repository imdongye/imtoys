//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <imgui.h>

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
#include "im_physics/app_phy2d.h"
#include "im_pbd/app_softbody_in_model.h"
#include "im_tests/app_scene3d.h"


using namespace lim;

static int selected_app_idx;
static std::vector<std::function<AppBase*()>> app_constructors;
static std::vector<const char*> app_names;
static std::vector<const char*> app_dirs;
static std::vector<const char*> app_infos;


static void drawAppSellector()
{
	static bool isSelectorOpened = false;
	const AppBase& app = *AppBase::g_ptr;

	// draw app selector
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
		for( int i = 0; i<app_names.size(); i++ ) {
			if( ImGui::Button(app_names[i]) ) {
				selected_app_idx = i;
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
	app_constructors.push_back( [](){ return new TApp(); } );
	app_names.push_back(TApp::APP_NAME);
	app_dirs.push_back(TApp::APP_DIR);
	app_infos.push_back( TApp::APP_INFO );
}


int main()
{
	pushAppData<AppScene3d>();
	pushAppData<AppSkeletal>();
	pushAppData<AppAstar>();
	pushAppData<AppSoftbodyInModel>();
	pushAppData<AppPbdCpu>();
	pushAppData<AppPbdGpu>();
	pushAppData<AppIK>();
	pushAppData<AppClothCPU>();
	pushAppData<AppClothGPU>();
	pushAppData<AppParticle>();
	pushAppData<AppCurve>();
	pushAppData<AppBvhParsor>();
	pushAppData<AppRay>();
	pushAppData<AppMineSweeper>();
	pushAppData<AppModelViewer>();
	pushAppData<AppSdfModeler>();
	pushAppData<AppTemplate>();
	pushAppData<AppImGuiTest>();
	pushAppData<AppICP>();
	pushAppData<AppGenMesh>();
	pushAppData<AppMovingWindow>();
	pushAppData<AppSimplification>();
	pushAppData<AppHatching>();
	pushAppData<AppHdr>();
	pushAppData<AppFluid>();
	pushAppData<AppShaderToy>();

	selected_app_idx = 0;

	if( app_names.size()>1 ) {
		AppBase::draw_appselector = drawAppSellector;
	}

	while( selected_app_idx>=0 )
	{
		AppBase::g_app_name = app_names[selected_app_idx];
		AppBase::g_app_dir = app_dirs[selected_app_idx];
		AppBase::g_app_info = app_infos[selected_app_idx];
		AppBase::g_ptr = app_constructors[selected_app_idx]();

		selected_app_idx = -1;

		AppBase::g_ptr->run();

		delete AppBase::g_ptr;
	}

	return 0;
}