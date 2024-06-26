#include <limbrary/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <limbrary/asset_lib.h>

using namespace lim;

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
BoneNode* cur_bone = nullptr; // temp used in app_skeletal.cpp
static RdNode::MsSet* cur_msset = nullptr;
static const char* me_inspector_name = "inspector";
static const char* me_hierarchy_name = "hierarchy";
static const char* me_animator_name  = "animator";
static const char* dl_name			 = "d_light editor";
static const char* dl_shadow_map_name= "d_light shadow map";

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
	ImGui::Begin(me_inspector_name);
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
		ImGui::Text("bone_idx : %d", nd.bone_idx);
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
	
	ImGui::Begin(me_animator_name);
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

	ImGui::Begin(me_hierarchy_name);
	drawHierarchy(md.root);
	ImGui::Separator();
	drawHierarchy(md.animator.bone_root);
	ImGui::End();

	drawInspector();

	drawAnimator(md.animator);
}
void LimGui::ModelEditorReset(const char* hname, const char* iname, const char* aname) {
	cur_md = nullptr;
	me_hierarchy_name = hname;
	me_inspector_name = iname;
	me_animator_name = aname;
}


void LimGui::LightDirectionalEditor(lim::LightDirectional& lit) {
	const static float lit_theta_spd = 70 * 0.001f;
	const static float lit_phi_spd = 360 * 0.001f;
	const static float lit_dist_spd = 45.f * 0.001f;
	static bool is_light_draged = false;
	static bool is_draw_shadow_map_view = false;
	ImGui::Begin(dl_name);
	is_light_draged |= ImGui::DragFloat("phi", &lit.tf.phi, lit_phi_spd, -FLT_MAX, +FLT_MAX, "%.3f");
	is_light_draged |= ImGui::DragFloat("theta", &lit.tf.theta, lit_theta_spd, 0, 80, "%.3f");
	is_light_draged |= ImGui::DragFloat("dist", &lit.tf.dist, lit_dist_spd, 5.f, 50.f, "%.3f");
	if( is_light_draged ) {
		lit.tf.updateWithRotAndDist();
	}
	ImGui::Text("pos: %.1f %.1f %.1f", lit.tf.pos.x, lit.tf.pos.y, lit.tf.pos.z);
	ImGui::SliderFloat("intencity", &lit.Intensity, 0.5f, 200.f, "%.1f");
	ImGui::SliderFloat2("light radius", &lit.shadow->RadiusUv.x, 0.f, 0.1f, "%.3f");
	ImGui::Checkbox("shadow enabled", &lit.shadow->Enabled);
	ImGui::Checkbox("show shadow map", &is_draw_shadow_map_view);
	if(is_draw_shadow_map_view && lit.shadow->Enabled) {
		Viewport& vp = AssetLib::get().texture_viewer; // todo 
		vp.getFb().bind();
		drawTexToQuad(lit.shadow->map.getRenderedTexId(), 2.2, 0.f, 1.f);
		vp.getFb().unbind();
		vp.drawImGui();
	}
	ImGui::End();
}
void LimGui::LightDirectionalEditorReset(const char* name, const char* smName) {
	dl_name = name;
	dl_shadow_map_name = smName; // not used
	AssetLib::get().texture_viewer.name = smName;
}
