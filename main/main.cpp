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
static bool _is_app_selected = true;
static char _app_selector_name[64];
static vector<function<lim::AppBase*()>> _app_constructors;
static vector<const char*> _app_names;
static vector<const char*> _app_descriptions;
int _selected_app_idx;


template <typename TApp>
static void pushAppData()
{
	_app_names.push_back(TApp::APP_NAME);
	_app_descriptions.push_back(TApp::APP_DESCRIPTION);
	_app_constructors.push_back([](){ return new TApp(); });
}

static void selectApp(int idx)
{
	_is_app_selected = true;
	_selected_app_idx = idx;
	strcpy(_app_selector_name, "AppSelector##");
	strcat(_app_selector_name, _app_names[idx]);
}

static void drawAppSellector()
{
	ImGui::Begin(_app_selector_name);
	for (int i = 0; i < _app_names.size(); i++)
	{
		if (ImGui::Button(_app_names[i]))
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

	if(_app_names.size()>1)
		lim::AppBase::_draw_appselector = drawAppSellector;


	while (_is_app_selected)
	{
		_is_app_selected = false;

		app = _app_constructors[_selected_app_idx]();

		app->run();

		delete app;
	}

	return 0;
}