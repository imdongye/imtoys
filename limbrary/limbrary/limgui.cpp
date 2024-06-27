#include <limbrary/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <limbrary/asset_lib.h>

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
BoneNode* cur_bone = nullptr;
static RdNode::MsSet* cur_msset = nullptr;

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
			cur_nd = &nd;
			cur_bone = nullptr;
			cur_msset = nullptr;
		}
		for(auto& msset: nd.meshs_mats) {
			ImGuiTreeNodeFlags msflags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
			if( cur_msset == &msset ) {
				msflags |= ImGuiTreeNodeFlags_Selected;
			}
			ImGui::TreeNodeEx(&msset, msflags, "%s", msset.ms->name.c_str());
			if( cur_msset!=&msset && ImGui::IsItemClicked(0) ) {
				cur_nd = nullptr;
				cur_bone = nullptr;
				cur_msset = &msset;
			}
			ImGui::TreePop();
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}
static int display_BoneIdx = 0;
static std::function<void(const lim::Program&)> makeSetProg() {
	return [&](const Program& prog) {
		prog.setUniform("display_BoneIdx", display_BoneIdx);
	};
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
			if( nd.bone_idx>=0 ) {
				display_BoneIdx = nd.bone_idx;
			} else {
				display_BoneIdx = -2;
			}
			cur_md->md_data->setSetProgToAllMat(makeSetProg());
			cur_nd = nullptr;
			cur_bone = &nd;
			cur_msset = nullptr;
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}
static void drawInspector() {
	ImGui::Begin("inspector");
	if( cur_nd ) {
		RdNode& nd = *cur_nd;
		ImGui::Text("name : %s", nd.name.c_str());
		ImGui::Text("childs : %d", nd.childs.size());
		ImGui::Checkbox("enabled", &nd.enabled);
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
			cur_md->animator.updateMtxBones();
		}
		LimGui::Mat4(nd.tf.mtx);
		ImGui:Text("bone_idx : %d", nd.bone_idx);
		if( nd.bone_idx>=0 ) {
			LimGui::Mat4(cur_md->md_data->bone_offsets[nd.bone_idx]);
		}
	}
	else if( cur_msset ) {
		const Mesh* ms = cur_msset->ms;
		const Material* mat = cur_msset->mat;
		ImGui::Text("name : %s", ms->name.c_str());
		ImGui::Text("material : %s", mat->name.c_str());
		ImGui::Checkbox("enabled", &cur_msset->enabled);
	}
	ImGui::End();
}
static void drawAnimator(Animator& animator) {
	if( animator.cur_anim == nullptr )
		return;
	
	ImGui::Begin("animator");
	ImGui::Text("#bones in mesh : %d", animator.mtx_Bones.size());
	switch(animator.state) {
	case Animator::State::PLAY: ImGui::Text("state : PLAY"); break;
	case Animator::State::PAUSE: ImGui::Text("state : PAUSE"); break;
	case Animator::State::STOP: ImGui::Text("state : STOP"); break;
	}
	float progress = animator.elapsed_sec / animator.duration_sec; 
	ImGui::ProgressBar(progress);
	ImGui::Text("%d/%d", (int)animator.cur_tick, (int)animator.cur_anim->nr_ticks);

	if(ImGui::Button("Play")) {
		animator.play();
	}
	ImGui::SameLine();
	if(ImGui::Button("Pause")) {
		animator.pause();
	}
	ImGui::SameLine();
	if(ImGui::Button("Stop")) {
		animator.stop();
	}
	ImGui::SameLine();
	ImGui::Checkbox("isLoop", &animator.is_loop);

	// if(ImGui::Button("default")) {
	// 	animator.updateDefaultMtxBones();
	// }
	std::vector<const char*> anim_list;
	for(auto& anim : animator.md_data->animations) {
		anim_list.push_back(anim.name.c_str());
	}
	static int anim_idx = 0;
	if(ImGui::Combo("animation", &anim_idx, anim_list.data(), anim_list.size())) {
		animator.setAnim(&animator.md_data->animations[anim_idx]);
	}
	ImGui::Text("#tracks in anim : %d", animator.cur_anim->tracks.size());
	ImGui::End();
}

void LimGui::ModelEditor(ModelView& md) {
	if( cur_md != &md ) {
		cur_md = &md;
		cur_nd = nullptr;
		cur_bone = nullptr;
		cur_msset = nullptr;
	}

	ImGui::Begin("hierarchy");
	drawHierarchy(md.root);
	ImGui::Separator();
	drawHierarchy(md.animator.bone_root);
	ImGui::End();

	drawInspector();

	drawAnimator(md.animator);
}
void LimGui::ModelEditorReset() {
	cur_md = nullptr;
}