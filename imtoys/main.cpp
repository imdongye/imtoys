//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include "limbrary/limclude.h"


#include "imhdr/app_hdr.h"
#include "imsimplification/app_simplification.h"
#include "impbr/app_pbr.h"
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
	static std::string selectorName = "AppSelector##app"+std::to_string(lim::AppPref::get().selectedAppIdx);

	ImGui::Begin(selectorName.c_str());
	for( int i=0; i<appNames.size(); i++ ) {
		if( ImGui::Button(appNames[i]) ) {
			appSelected=true;
			lim::AppPref::get().selectedAppIdx=i;
			selectorName = "AppSelector##app"+std::to_string(lim::AppPref::get().selectedAppIdx);
			demo_window_name = selectorName.c_str();
			glfwSetWindowShouldClose(app->window, true);
		}
	}
	ImGui::End();
}

// rid unused variables warnings
int main(int, char**)
{
	std::string demoWindowName;
	lim::imgui_modules::draw_appselector = drawAppSellector;
    pushAppData<lim::AppHdr>();
    pushAppData<lim::AppSimplification>();
	pushAppData<lim::AppPbr>();
	//pushAppData<lim::AppSnell>();


	while( appSelected ) {
		appSelected = false;
		int appIdx = lim::AppPref::get().selectedAppIdx;
		app = appConstructors[appIdx]();
		lim::AppPref::get().selectedAppName = appNames[appIdx];
		demoWindowName = "Dear ImGui Demo##demo"+std::to_string(appIdx);
		demo_window_name = demoWindowName.c_str();

		app->run();

		delete app;
	}

	return 0;
}
