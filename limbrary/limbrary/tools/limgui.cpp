#include <limbrary/tools/limgui.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/general.h>
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






namespace {
	ModelView* cur_md = nullptr;
	RdNode*	  picked_nd = nullptr;
	BoneNode* picked_bone = nullptr; // temp used in app_skeletal.cpp
	const char* model_editor_window_name = "ModelEditor";
}

void LimGui::ModelEditorReset(const char* windowName) {
	cur_md = nullptr;
	model_editor_window_name = windowName;
}


static void drawHierarchy(RdNode& nd) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( picked_nd == &nd ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( nd.childs.empty() ) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	}
	
	if(ImGui::TreeNodeEx(&nd, flags, nd.name.c_str())) {
		if( picked_nd!=&nd && ImGui::IsItemClicked(0) ) {
			picked_nd = &nd;
			picked_bone = nullptr;
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}

static void drawHierarchy(std::vector<BoneNode>& skel, int curIdx, int curLevel=0) {
	BoneNode& curBone = skel[curIdx];
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( picked_bone == &curBone ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( curBone.nr_childs == 0 ) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		if( curLevel!=2 ) {
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}
	}
	
	if( ImGui::TreeNodeEx(&curBone, flags, curBone.name.c_str()) ) {
		if( picked_bone!=&curBone && ImGui::IsItemClicked(0) ) {
			picked_nd = nullptr;
			picked_bone = &curBone;
		}
		int nrFinded = 0;
		for( int i=curIdx+1; nrFinded<curBone.nr_childs; i++ ) {
			if( skel[i].idx_parent_bone_node == curIdx ) {
				nrFinded++;
				drawHierarchy(skel, i, curLevel+1);
			}
		}
		ImGui::TreePop();
	}
}

static void drawInspector() {
	if( picked_nd ) {
		ImGui::TextUnformatted("<RdNode>");
		RdNode& nd = *picked_nd;
		ImGui::Text("name : %s", nd.name.c_str());
		if( nd.parent ) {
			ImGui::Text("parrent name : %s", nd.parent->name.c_str());
		}
		ImGui::Text("#childs : %d", nd.childs.size());
		ImGui::Checkbox("enabled", &nd.enabled);
		ImGui::Checkbox("enabled", &nd.is_identity_mtx);
		ImGui::Checkbox("enabled", &nd.is_local_is_global);
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
		if( nd.ms ) {
			ImGui::TextUnformatted("<Mesh>");
			const Mesh& ms = *nd.ms;
			ImGui::Text("name : %s", ms.name.c_str());
			ImGui::Text("#tris: %d, #verts: %d", ms.nr_tris, ms.nr_verts);
			ImGui::Text("pos:%s,nor:%s,uv:%s,col:%s,tan:%s,bitan:%s,bone_info:%s"
				, boolStrOX(!ms.poss.empty()), boolStrOX(!ms.nors.empty()), boolStrOX(!ms.uvs.empty())
				, boolStrOX(!ms.cols.empty()), boolStrOX(!ms.tangents.empty())
				, boolStrOX(!ms.bitangents.empty()), boolStrOX(!ms.bone_infos.empty()));

			ImGui::TextUnformatted("<Material>");
			Material& mat = *(Material*)nd.mat; // warning : rid const
			ImGui::Text("name : %s", mat.name.c_str());
			ImGui::SliderFloat("shininess", &mat.Shininess, 0.5f, 300);
			ImGui::SliderFloat("roughness", &mat.Roughness, 0.015f, 1);
			ImGui::SliderFloat("metalness", &mat.Metalness, 0.000f, 1);
			ImGui::ColorEdit3("ambient color", &mat.AmbientColor[0]);

		}
	}
	else if( picked_bone ) {
		ImGui::TextUnformatted("<BoneNode>");
		BoneNode& nd = *picked_bone;
		ImGui::Text("name : %s", nd.name.c_str());
		ImGui::Text("childs : %d", nd.nr_childs);
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
			cur_md->own_animator->updateMtxBones();
		}
		LimGui::Mat4(nd.tf.mtx);
		ImGui::Text("bone_idx : %d", nd.idx_weighted_bone);
		if( nd.idx_weighted_bone>=0 ) {
			LimGui::Mat4(cur_md->md_data->weighted_bone_offsets[nd.idx_weighted_bone]);
		}
	}
	else {
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
		ImGui::Text("<nothing selected>");
		ImGui::Dummy(ImVec2(0.0f, 20.0f));
	}
}
static void drawAnimator(Animator& animator) {
	if( animator.cur_anim == nullptr )
		return;

	ImGui::Text("#bones in mesh : %d", animator.mtx_Bones.size());
	switch(animator.state) {
	case Animator::ST_PLAY: ImGui::Text("state : PLAY"); break;
	case Animator::ST_PAUSE: ImGui::Text("state : PAUSE"); break;
	case Animator::ST_STOP: ImGui::Text("state : STOP"); break;
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
}

void LimGui::ModelEditor(ModelView& md) {
	if( cur_md != &md ) {
		cur_md = &md;
		picked_nd = nullptr;
		picked_bone = nullptr;
	}
	ImGui::Begin(model_editor_window_name);
	if( ImGui::CollapsingHeader("Info") ) {
		ImGui::Text("name : %s", md.md_data->name.c_str());
		ImGui::Text("path : %s", md.md_data->path.c_str());
		if( ImGui::TreeNode("boundary") ) {
			ImGui::Text("min : %.2f %.2f %.2f", md.md_data->boundary_min.x, md.md_data->boundary_min.y, md.md_data->boundary_min.z);
			ImGui::Text("max : %.2f %.2f %.2f", md.md_data->boundary_max.x, md.md_data->boundary_max.y, md.md_data->boundary_max.z);
			ImGui::Text("size: %.2f %.2f %.2f", md.md_data->boundary_size.x, md.md_data->boundary_size.y, md.md_data->boundary_size.z);
			ImGui::TreePop();
		}
		if( ImGui::TreeNode("number of") ) {
			ImGui::Text("verteces : %d", md.md_data->total_verts);
			ImGui::Text("tris : %d", md.md_data->total_tris);
			ImGui::Text("meshes : %d", md.md_data->own_meshes.size());
			ImGui::Text("textures : %d", md.md_data->own_textures.size());
			ImGui::Text("materials : %d", md.md_data->own_materials.size());
			ImGui::TreePop();
		}
	}


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Inspactor") ) {
		drawInspector();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Render tree") ) {
		drawHierarchy(md.root);
	}
	if( ImGui::CollapsingHeader("Bone tree") ) {
		ImGui::Text("depth of bone root : %d", md.md_data->depth_of_bone_root_in_rdtree);
		drawHierarchy(md.own_animator->bones, 0);
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Animator") ) {
		drawAnimator(*md.own_animator);
	}
	ImGui::End();
}

lim::BoneNode* LimGui::getPickedBoneNode() {
	return picked_bone;
}

lim::RdNode* LimGui::getPickedRenderNode() {
	return picked_nd;
}



namespace {
	const char* dl_name			 = "d_light editor";
	const char* dl_shadow_map_name= "d_light shadow map";
}

void LimGui::LightDirectionalEditorReset(const char* name, const char* smName) {
	dl_name = name;
	dl_shadow_map_name = smName; // not used
	AssetLib::get().texture_viewer.name = smName;
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
		drawTexToQuad(lit.shadow->map.getRenderedTexId(), 2.2f, 0.f, 1.f);
		vp.getFb().unbind();
		vp.drawImGui();
	}
	ImGui::End();
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