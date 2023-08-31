//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <imgui.h>
#include <limbrary/imgui_module.h>
#include <limbrary/app_pref.h>
#include <imtests/app_template.h>
#include <imtests/app_icp.h>
#include "imtests/app_gen_mesh.h"
#include "imtests/app_font.h"
#include "imtests/app_astar.h"
#include "imsimplification/app_simplification.h"
#include "impbr/app_pbr.h"
#include "imnpr/app_hatching.h"
#include "imhdr/app_hdr.h"
#include "imanims/app_nano.h"
#include "imanims/app_kinematics.h"
#include "imanims/app_fluid.h"

lim::AppBase *app;

bool appSelected = true;
std::vector<std::function<lim::AppBase*()>> appConstructors;
std::vector<const char*> appNames;
std::vector<const char*> appDicripts;

template <class App>
void pushAppData();
void drawAppSellector();

// rid unused variables warnings
int main(int, char **)
{
	lim::ImguiModule::draw_appselector = drawAppSellector;

	// first order is shown first
	pushAppData<lim::AppTemplate>();
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

	lim::AppPref::get().selectedAppIdx = 0;
	lim::AppPref::get().selectedAppName = appNames[0];

	while (appSelected)
	{
		appSelected = false;
		int appIdx = lim::AppPref::get().selectedAppIdx;

		app = appConstructors[appIdx]();

		app->run();

		delete app;
	}

	return 0;
}

template <class App>
void pushAppData()
{
	appNames.push_back(App::APP_NAME);
	appDicripts.push_back(App::APP_DISC);
	appConstructors.push_back([]()
							  { return new App(); });
}

void drawAppSellector()
{
	static std::string selectorName = "AppSelector##app" + std::string(appNames[lim::AppPref::get().selectedAppIdx]);

	if (appNames.size() == 1)
		return;
	ImGui::Begin(selectorName.c_str());
	for (int i = 0; i < appNames.size(); i++)
	{
		// when button pushed
		if (ImGui::Button(appNames[i]))
		{
			appSelected = true;
			lim::AppPref::get().selectedAppIdx = i;
			lim::AppPref::get().selectedAppName = appNames[i];

			selectorName = "AppSelector##app";
			selectorName += std::string(appNames[i]);

			glfwSetWindowShouldClose(app->window, true);
		}
	}
	ImGui::End();
}
