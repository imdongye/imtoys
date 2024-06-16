#include <limbrary/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>

using namespace ImGui;
using namespace lim;

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

void LimGui::Mat4(glm::mat4& m) {
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
void LimGui::Vec3(glm::vec3& v) {
    ImGui::Text("%.2f %.2f %.2f", v.x, v.y, v.z);
}


static ModelView* cur_md = nullptr;
static RdNode* cur_nd = nullptr;
static BoneNode* cur_bone = nullptr;

static void drawHierarchy(RdNode& nd) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( cur_nd == &nd ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( nd.childs.size() == 0 ) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	}
	
	if(ImGui::TreeNodeEx(&nd, flags, nd.name.c_str())) {
		if( cur_nd!=&nd && ImGui::IsItemClicked(0) ) {
			cur_bone = nullptr;
			cur_nd = &nd;
		}
		for(auto& [ms, mat]: nd.meshs_mats) {
			ImGui::BulletText("%s\n%s", ms->name.c_str(), mat->name.c_str());
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}
static void drawHierarchy(BoneNode& nd) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( cur_bone == &nd ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( nd.childs.size() == 0 ) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	}
	
	if(ImGui::TreeNodeEx(&nd, flags, nd.name.c_str())) {
		if( cur_bone!=&nd && ImGui::IsItemClicked(0) ) {
			cur_nd = nullptr;
			cur_bone = &nd;
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}
static void drawInspector() {
	if( cur_nd ) {
		RdNode& nd = *cur_nd;
		ImGui::Text("name : %s", nd.name.c_str());
		ImGui::Text("childs : %d", nd.childs.size());
		bool edited = false;
		edited |= ImGui::DragFloat3("pos", glm::value_ptr(nd.tf.pos), 0.01f);
		edited |= ImGui::DragFloat3("scale", glm::value_ptr(nd.tf.scale), 0.01f);
		glm::vec3 rot = glm::degrees(glm::eulerAngles(nd.tf.ori));
		if(ImGui::DragFloat3("ori", glm::value_ptr(rot), 0.1f)) {
			edited = true;
			nd.tf.ori = glm::quat(glm::radians(rot));
		}
		if( edited ) {
			nd.tf.update();
		}
		LimGui::Mat4(nd.tf.mtx);
	}
	else if( cur_bone ) {
		BoneNode& nd = *cur_bone;
		ImGui::Text("name : %s", nd.name.c_str());
		ImGui::Text("childs : %d", nd.childs.size());
		bool edited = false;
		edited |= ImGui::DragFloat3("pos", glm::value_ptr(nd.tf.pos), 0.01f);
		edited |= ImGui::DragFloat3("scale", glm::value_ptr(nd.tf.scale), 0.01f);
		glm::vec3 rot = glm::degrees(glm::eulerAngles(nd.tf.ori));
		if(ImGui::DragFloat3("ori", glm::value_ptr(rot), 0.1f)) {
			edited = true;
			nd.tf.ori = glm::quat(glm::radians(rot));
		}
		if( edited ) {
			nd.tf.update();
		}
		LimGui::Mat4(nd.tf.mtx);
	}
}
static void drawAnimator(ModelView& md) {
	ImGui::Text("#bones in mesh : %d", md.animator.mtx_Bones.size());
	ImGui::Text("#bones in anim : %d", md.animator.cur_anim->tracks.size());
	switch(md.animator.state) {
	case Animator::State::PLAY: ImGui::Text("state : PLAY"); break;
	case Animator::State::PAUSE: ImGui::Text("state : PAUSE"); break;
	case Animator::State::STOP: ImGui::Text("state : STOP"); break;
	}
	float progress = md.animator.elapsed_sec / md.animator.duration_sec; 
	ImGui::ProgressBar(progress);


	if(ImGui::Button("Play")) {
		md.animator.play(0);
	}
	ImGui::SameLine();
	if(ImGui::Button("Pause")) {
		md.animator.pause();
	}
	ImGui::SameLine();
	if(ImGui::Button("Stop")) {
		md.animator.stop();
	}
	ImGui::SameLine();
	ImGui::Checkbox("isLoop", &md.animator.is_loop);

	// if(ImGui::Button("default")) {
	// 	md.animator.updateDefaultMtxBones();
	// }
	std::vector<const char*> anim_list;
	for(auto& anim : md.md_data->animations) {
		anim_list.push_back(anim.name.c_str());
	}
	static int anim_idx = 0;
	if(ImGui::Combo("animation", &anim_idx, anim_list.data(), anim_list.size())) {
		md.animator.play(&md.md_data->animations[anim_idx]);
	}
}

void LimGui::ModelEditor(ModelView& md) {
	if( cur_md != &md ) {
		cur_md = &md;
		cur_nd = nullptr;
		cur_bone = nullptr;
	}

	ImGui::Begin("hierarchy");
	drawHierarchy(md.root);
	drawHierarchy(md.animator.bone_root);
	ImGui::End();

	ImGui::Begin("inspector");
	drawInspector();
	ImGui::End();

	ImGui::Begin("animator");
	drawAnimator(md);
	ImGui::End();
}