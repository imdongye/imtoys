//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <limbrary/tools/apps_selector.h>
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
#include "im_anims/app_ik.h"
#include "im_anims/app_curve.h"
#include "im_physics/app_phy2d.h"
#include "im_tests/app_scene3d.h"

#ifndef __APPLE__
#include "im_anims/app_particle.h"
#include "im_anims/app_cloth_gpu.h"
#include "im_pbd/app_pbd_cpu.h"
#include "im_pbd/app_pbd_gpu.h"
#include "im_pbd/app_softbody_in_model.h"
#endif


using namespace lim;



int main()
{
	apps_selector::add<AppScene3d>();
	apps_selector::add<AppSkeletal>();
	apps_selector::add<AppAstar>();
	apps_selector::add<AppIK>();
	apps_selector::add<AppClothCPU>();
	apps_selector::add<AppCurve>();
	apps_selector::add<AppBvhParsor>();
	apps_selector::add<AppRay>();
	apps_selector::add<AppMineSweeper>();
	apps_selector::add<AppModelViewer>();
	apps_selector::add<AppSdfModeler>();
	apps_selector::add<AppTemplate>();
	apps_selector::add<AppImGuiTest>();
	apps_selector::add<AppICP>();
	apps_selector::add<AppGenMesh>();
	apps_selector::add<AppMovingWindow>();
	apps_selector::add<AppSimplification>();
	apps_selector::add<AppHatching>();
	apps_selector::add<AppHdr>();
	apps_selector::add<AppFluid>();
	apps_selector::add<AppShaderToy>();
#ifndef __APPLE__
	apps_selector::add<AppSoftbodyInModel>();
	apps_selector::add<AppClothGPU>();
	apps_selector::add<AppParticle>();
	apps_selector::add<AppPbdCpu>();
	apps_selector::add<AppPbdGpu>();
#endif

	apps_selector::wanted_app_idx = 0;

	apps_selector::run();


	//
	// without apps_selector ( for one app )
	//
	// AppBase::g_app_name = AppTemplate::APP_NAME;
	// AppBase::g_app_dir =  AppTemplate::APP_DIR;
	// AppBase::g_app_info = AppTemplate::APP_INFO;
	// AppTemplate app;
	// app.run();


	return 0;
}