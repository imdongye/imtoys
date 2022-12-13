//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include "limbrary/limclude.h"

#include "imhdr/app_hdr.h"
#include "imsnells/app_snell.h"
#include "impbr/app_pbr.h"
#include "imsimplification/app_simplification.h"


lim::AppBase *app;

bool appSelected=true;
int selectedAppIdx=3;
std::vector<std::function<lim::AppBase*()>> appConstructors;
std::vector<const char*> appNames;
std::vector<const char*> appDicripts;

template<class App>
void pushAppData()
{
	appNames.push_back(App::APP_NAME);
	appDicripts.push_back(App::APP_DISC);
	appConstructors.push_back([]() { return new App(); });
}

void drawAppSellector()
{
	static std::string selectorName = lim::fmToStr("AppSelector%d", selectedAppIdx);

	ImGui::Begin(selectorName.c_str());
	for( int i=0; i<appNames.size(); i++ ) {
		if( ImGui::Button(appNames[i]) ) {
			appSelected=true;
			selectedAppIdx=i;
			selectorName = lim::fmToStr("AppSelector%d", selectedAppIdx);
			glfwSetWindowShouldClose(app->window, true);
		}
	}
	ImGui::End();
}

// rid unused variables warnings
int main(int, char**)
{
	lim::imgui_modules::draw_appselector = drawAppSellector;

	pushAppData<lim::AppHdr>();
	pushAppData<lim::AppSnell>();
	pushAppData<lim::AppPbr>();
	pushAppData<lim::AppSimplification>();

	while( appSelected ) {
		appSelected = false;
		app = appConstructors[selectedAppIdx]();

		app->run();

		delete app;
	}

	return 0;
}
