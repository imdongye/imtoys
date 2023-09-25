//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <imgui.h>
#include <limbrary/asset_lib.h>
#include <limbrary/app_pref.h>
#include <limbrary/log.h>
#include <limbrary/viewport.h>

#include "im_tests/app_template.h"
#include "im_tests/app_imgui_test.h"
#include "im_tests/app_icp.h"
#include "im_tests/app_gen_mesh.h"
#include "im_tests/app_font.h"
#include "im_tests/app_astar.h"
#include "im_simplification/app_simplification.h"
#include "im_pbr/app_pbr.h"
#include "im_npr/app_hatching.h"
#include "im_hdr/app_hdr.h"
#include "im_anims/app_nano.h"
#include "im_anims/app_kinematics.h"
#include "im_anims/app_fluid.h"
#include "im_model_viewer/app_model_viewer.h"

using namespace std;


static lim::AppBase *app;
static bool appSelected = true;
static char appSelectorName[64];
static vector<function<lim::AppBase*()>> appConstructors;
static vector<const char*> appNames;
static vector<const char*> appDicripts;


template <class App>
static void pushAppData()
{
	appNames.push_back(App::APP_NAME);
	appDicripts.push_back(App::APP_DISC);
	appConstructors.push_back([](){ return new App(); });
}


static void selectApp(int idx)
{
	appSelected = true;
	lim::AppPref::get().selected_app_idx = idx;
	lim::AppPref::get().selected_app_name = appNames[idx];
	strcpy(appSelectorName, "AppSelector##");
	strcat(appSelectorName, appNames[idx]);
}


static void drawAppSellector()
{
	ImGui::Begin(appSelectorName);
	for (int i = 0; i < appNames.size(); i++)
	{
		if (ImGui::Button(appNames[i]))
		{
			selectApp(i);
			glfwSetWindowShouldClose(app->window, true);
		}
	}
	ImGui::End();
}


// rid unused variables warnings
int main(int, char **)
{
	pushAppData<lim::AppTemplate>();
	pushAppData<lim::AppImGuiTest>();
	pushAppData<lim::AppICP>();
	pushAppData<lim::AppGenMesh>();
	pushAppData<lim::AppFont>();
	pushAppData<lim::AppAstar>();
	pushAppData<lim::AppSimplification>();
	pushAppData<lim::AppPbr>();
	pushAppData<lim::AppHatching>();
	pushAppData<lim::AppHdr>();
	pushAppData<lim::AppNano>();
	pushAppData<lim::AppKinematics>();
	pushAppData<lim::AppFluid>();
	pushAppData<lim::AppModelViewer>();

	selectApp(0);

	if(appNames.size()>1)
		lim::AppBase::_draw_appselector = drawAppSellector;


	while (appSelected)
	{
		appSelected = false;
		int appIdx = lim::AppPref::get().selected_app_idx;

		app = appConstructors[appIdx]();

		app->run();

		delete app;
	}

	return 0;
}