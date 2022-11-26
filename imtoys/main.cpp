//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

/* for vsprintf_s */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lim/limclude.h"


lim::AppBase *app;

int selectedAppIdx=1;
std::vector<std::function<lim::AppBase*()>> app_constructors;
std::vector<const char*> app_names;
std::vector<const char*> app_discripts;

template<class App>
void pushAppData()
{
	app_names.push_back(App::APP_NAME);
	app_discripts.push_back(App::APP_DISC);
	app_constructors.push_back([]() { return new App(); });
}

void drawAppSellector()
{
	ImGui::Begin("AppSelector");
	for( int i=0; i<app_names.size(); i++ ) {
		if( ImGui::Button(app_names[i]) ) {
			selectedAppIdx=i;
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

	app = app_constructors[selectedAppIdx]();
	int tempAppIdx;
	while( !glfwWindowShouldClose(app->window) ) {
		tempAppIdx = selectedAppIdx;

		app->run();

		if( tempAppIdx != selectedAppIdx ) {
			delete app;
			app = app_constructors[selectedAppIdx]();
		}
	}

	delete app;

	return 0;
}
