#include <limbrary/tools/apps_selector.h>
#include <imgui.h>

using namespace lim;

int apps_selector::wanted_app_idx = 0;
int apps_selector::selected_app_idx = -1;
int apps_selector::nr_apps = 0;
std::vector<const char*> apps_selector::app_names;
std::vector<const char*> apps_selector::app_dirs;
std::vector<const char*> apps_selector::app_infos;
std::vector<std::function<AppBase*()>> apps_selector::app_constructors;

void apps_selector::__drawGui()
{
	static bool is_opened = false;
	const AppBase& app = *AppBase::g_ptr;

	// draw app selector
	if( ImGui::IsKeyPressed(ImGuiKey_F1, false) ) {
		if( !is_opened ) {
			is_opened = true;
			ImGui::OpenPopup("AppSelector");
		}
		else {
			is_opened = false;
		}
	}

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if( ImGui::BeginPopupModal("AppSelector", &is_opened, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize) ) {
		for( int i = 0; i<app_names.size(); i++ ) {
			if( ImGui::Button(app_names[i]) ) {
				wanted_app_idx = i;
				is_opened = false;
				ImGui::CloseCurrentPopup();
				glfwSetWindowShouldClose(app.window, true);
			}
		}
		ImGui::EndPopup();
	}	
}