#include <limbrary/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>

using namespace ImGui;


bool LimGui::CheckBox3(const char* label, bool v[3])
{
    static const char* check_box_labels[3] = {"X", "Y", "Z"};
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(3, CalcItemWidth());
    for (int i = 0; i < 3; i++)
    {
        PushID(i);
        value_changed |= Checkbox(check_box_labels[i], &v[i]);
        float itemWidth = GetItemRectSize().x;
        float sectionWidth = CalcItemWidth() + g.Style.ItemInnerSpacing.x;
        SameLine(0, sectionWidth+ - itemWidth);
        PopID();
        PopItemWidth();
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}