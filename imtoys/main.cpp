//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include "limbrary/limclude.h"


#include "imnpr/app_hatching.h"
#include "imhdr/app_hdr.h"
#include "imsimplification/app_simplification.h"
#include "impbr/app_pbr.h"
#include "imtests/app_astar.h"
//#include "imsnells/app_snell.h"

lim::AppBase *app;

bool appSelected=true;
std::vector<std::function<lim::AppBase*()>> appConstructors;
std::vector<const char*> appNames;
std::vector<const char*> appDicripts;

extern const char *demo_window_name;

template<class App>
void pushAppData()
{
	appNames.push_back(App::APP_NAME);
	appDicripts.push_back(App::APP_DISC);
	appConstructors.push_back([]() { return new App(); });
}

void drawAppSellector()
{
	static std::string selectorName = "AppSelector##app"+std::string(appNames[lim::AppPref::get().selectedAppIdx]);

	ImGui::Begin(selectorName.c_str());
	for( int i=0; i<appNames.size(); i++ ) {
		// when button pushed
		if( ImGui::Button(appNames[i]) ) {
			appSelected=true;
			lim::AppPref::get().selectedAppIdx=i;
			lim::AppPref::get().selectedAppName = appNames[i];

			selectorName = "AppSelector##app"; 
			selectorName += std::string(appNames[i]);

			glfwSetWindowShouldClose(app->window, true);
		}
	}
	ImGui::End();
}

// rid unused variables warnings
int main(int, char**)
{
	lim::imgui_modules::draw_appselector = drawAppSellector;

	pushAppData<lim::AppHatching>();
	pushAppData<lim::AppPbr>();
	pushAppData<lim::AppAstar>();
    pushAppData<lim::AppHdr>();
    pushAppData<lim::AppSimplification>();
	//pushAppData<lim::AppSnell>();

	lim::AppPref::get().selectedAppIdx=0;
	lim::AppPref::get().selectedAppName = appNames[0];

	while( appSelected ) {
		appSelected = false;
		int appIdx = lim::AppPref::get().selectedAppIdx;

		std::string demoWindowName = "Dear ImGui Demo##demo";
		demoWindowName += std::string(appNames[appIdx]);
		demo_window_name = demoWindowName.c_str();

		app = appConstructors[appIdx]();

		app->run();

		delete app;
	}

	return 0;
}
