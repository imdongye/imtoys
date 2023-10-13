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
#include "im_shadertoy/app_shadertoy.h"

static int _selected_app_idx;
static const char* const _app_selector_name = "AppSelector";

static std::vector<std::function<lim::AppBase*()>> _app_constructors;
static std::vector<const char*> _app_names;
static std::vector<const char*> _app_descriptions;


template <typename TApp>
static void pushAppData()
{
	_app_names.push_back(TApp::APP_NAME);
	_app_descriptions.push_back(TApp::APP_DESCRIPTION);
	_app_constructors.push_back([](){ return new TApp(); });
}

static void drawAppSellector()
{
	static bool isSelectorOpened = false;
	if( ImGui::IsKeyPressed(ImGuiKey_F1, false) ) {
		if( !isSelectorOpened ) {
			isSelectorOpened = true;
			ImGui::OpenPopup(_app_selector_name);
		}
		else {
			isSelectorOpened = false;
		}
	}

	if( ImGui::BeginPopupModal(_app_selector_name, &isSelectorOpened, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize) ) {
		for( int i = 0; i<_app_names.size(); i++ ) {
			if( ImGui::Button(_app_names[i]) ) {
				_selected_app_idx = i;
				glfwSetWindowShouldClose(lim::AppPref::get().app->window, true);
			}
		}
		ImGui::EndPopup();
	}

	// frame rate debugger
	static bool isFpsOpened = false;
	if( ImGui::IsKeyPressed(ImGuiKey_F2, false) )
		isFpsOpened = !isFpsOpened;
    if( isFpsOpened ) {
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

		const float PAD = 10.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 workPos = viewport->WorkPos;
		ImVec2 windowPos = {workPos.x+PAD, workPos.y+PAD+PAD};
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		if( ImGui::Begin("Example: Simple overlay", &isFpsOpened, window_flags) )
		{
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::Separator();
			if( ImGui::IsMousePosValid() )
				ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
			else
				ImGui::Text("Mouse Position: <invalid>");
		}
		ImGui::End();
	}
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
	pushAppData<lim::AppShaderToy>();

	_selected_app_idx = 0;

	if(_app_names.size()>1)
		lim::AppBase::_draw_appselector = drawAppSellector;

	while (_selected_app_idx>=0)
	{
		lim::AppBase* app = _app_constructors[_selected_app_idx]();
		_selected_app_idx = -1;

		app->run();

		delete app;
	}

	return 0;
}