#include <limbrary/tools/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <limbrary/tools/apps_selector.h>
#include <limbrary/tools/log.h>
#include <map>


using namespace lim;

static glm::uint genHash(const char *string)
{
    glm::uint hash = 0;
    while( char c = *string++)
    {
       hash = 65599 * hash + c;
    }
    return hash ^ (hash >> 16);
}

bool LimGui::CheckBox3(const char* label, bool v[3])
{
    static const char* check_box_labels[3] = {"X", "Y", "Z"};
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    for (int i = 0; i < 3; i++)
    {
        ImGui::PushID(i);
        value_changed |= ImGui::Checkbox(check_box_labels[i], &v[i]);
        float itemWidth = ImGui::GetItemRectSize().x;
        float sectionWidth = ImGui::CalcItemWidth() + g.Style.ItemInnerSpacing.x;
        ImGui::SameLine(0, sectionWidth+ - itemWidth);
        ImGui::PopID();
        ImGui::PopItemWidth();
    }
    ImGui::PopID();

    const char* label_end = ImGui::FindRenderedTextEnd(label);
    if (label != label_end)
    {
        ImGui::TextEx(label, label_end);
    }

    ImGui::EndGroup();
    return value_changed;
}

void LimGui::Mat4(const glm::mat4& m) {
    static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
    if( ImGui::BeginTable("matrix", 4, flags) ) {
        for( int i = 0; i < 4; i++ ) {
            ImGui::TableNextRow();
            for( int j = 0; j < 4; j++ ) {
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", m[j][i]);
            }
        }
        ImGui::EndTable();
    }
}
void LimGui::Vec3(const glm::vec3& v) {
    ImGui::Text("%.2f %.2f %.2f", v.x, v.y, v.z);
}




struct PlotVarData {
	int insert_idx = -1;
	std::vector<float> data;
};
static std::map<glm::uint, PlotVarData> g_plot_var_map;

void LimGui::PlotVal(const char* name, const char* postFix, float value, int bufSize) {
	ImGui::PushID(name);
	// ImGuiID id = ImGui::GetID("");

	auto myId = genHash(name);
	
	PlotVarData& pvd = g_plot_var_map[myId];
	if( pvd.insert_idx<0 ) {
		pvd.insert_idx = 0;
		pvd.data.resize(bufSize, 0);
	}
	assert(pvd.data.size() == bufSize);
	if( pvd.insert_idx == bufSize ) {
		pvd.insert_idx = 0;
	}
	pvd.data[pvd.insert_idx++] = value;
	ImGui::Text("%s\t%-3.4f %s", name, value, postFix);
	ImGui::PlotLines("##plotvar", pvd.data.data(), bufSize, pvd.insert_idx, nullptr, FLT_MAX, FLT_MAX, {0, 50});
	ImGui::PopID();
}



void LimGui::PlotVal(const char* name, const char* postFix, int bufSize) {
	// ImGui::PushID(name);
	// ImGuiID id = ImGui::GetID("");

	auto myId = genHash(name);

	
	PlotVarData& pvd = g_plot_var_map[myId];
	if( pvd.insert_idx<0 ) {
		pvd.insert_idx = 1;
		pvd.data.resize(bufSize, 0);
	}
	assert(pvd.data.size() == bufSize);
	ImGui::Text("%s\t%-3.4f %s", name, pvd.data[pvd.insert_idx-1], postFix);
	ImGui::PlotLines("##plotvar", pvd.data.data(), bufSize, pvd.insert_idx, nullptr, FLT_MAX, FLT_MAX, {0, 50});
	// ImGui::PopID();
}

void LimGui::PlotValAddValue(const char* name, float value, int bufSize) {


	auto myId = genHash(name);

	PlotVarData& pvd = g_plot_var_map[myId];
	if( pvd.insert_idx<0 ) {
		pvd.insert_idx = 0;
		pvd.data.resize(bufSize, 0);
	}
	assert(pvd.data.size() == bufSize);
	if( pvd.insert_idx == bufSize ) {
		pvd.insert_idx = 0;
	}
	pvd.data[pvd.insert_idx++] = value;
}






