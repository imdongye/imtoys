
#define IMGUI_DEFINE_MATH_OPERATORS
#include <limbrary/tools/limgui.h>
#include <imgui_internal.h>
#include <imguizmo/ImGuizmo.h>

#include <limbrary/application.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/text.h>
#include <limbrary/using_in_cpp/std.h>
#include <map>

using namespace lim;



namespace {
	Model*              picked_md_data = nullptr;
	ModelView*          picked_md_view = nullptr;
	RdNode*	            picked_rd_node = nullptr;
	BoneNode*           picked_bone_node = nullptr;
    LightDirectional*   picked_dir_lit = nullptr;
    LightSpot*          picked_spot_lit = nullptr;
    LightOmni*          picked_omni_lit = nullptr;
	IBLight*            picked_ib_light = nullptr;

	string md_view_editor_win_name = "ModelView##appname____";
	string md_data_editor_win_name = "ModelData##appname____";
	string rd_node_editor_win_name = "RenderNode##appname____";
	string bone_node_editor_win_name = "BoneNode##appname____";
	string light_editor_window_name = "Light##appname____";
	string scene_editor_window_name = "Scene##appname____";
}

void LimGui::resetEditors()
{
	picked_md_data = nullptr;
    picked_md_view = nullptr;
    picked_rd_node = nullptr;
    picked_bone_node = nullptr;
    picked_dir_lit = nullptr;
    picked_spot_lit = nullptr;
    picked_omni_lit = nullptr;

    md_view_editor_win_name = fmtStrToBuf("ModelView##%s", AppBase::g_app_name);
    md_data_editor_win_name = fmtStrToBuf("ModelData##%s", AppBase::g_app_name);
    rd_node_editor_win_name = fmtStrToBuf("RenderNode##%s", AppBase::g_app_name);
    bone_node_editor_win_name = fmtStrToBuf("BoneNode##%s", AppBase::g_app_name);
	light_editor_window_name = fmtStrToBuf("Light##%s", AppBase::g_app_name);
	scene_editor_window_name = fmtStrToBuf("Scene##%s", AppBase::g_app_name);
}



//
//	Model View Editor
//
static void drawHierarchy(RdNode& nd)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( picked_rd_node == &nd ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( nd.childs.empty() ) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	}
	
	if(ImGui::TreeNodeEx(&nd, flags, nd.name.c_str())) {
		if( picked_rd_node!=&nd && ImGui::IsItemClicked(0) ) {
			picked_rd_node = &nd;
			picked_bone_node = nullptr;
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}
}

static void drawHierarchy(std::vector<BoneNode>& skel, int curIdx, int curLevel=0)
{
	BoneNode& curBone = skel[curIdx];
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( picked_bone_node == &curBone ) {
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
		if( picked_bone_node!=&curBone && ImGui::IsItemClicked(0) ) {
			picked_rd_node = nullptr;
			picked_bone_node = &curBone;
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

static void drawAnimator(Animator& animator)
{
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
	for(auto& anim : animator.src_md->animations) {
		anim_list.push_back(anim.name.c_str());
	}
	static int anim_idx = 0;
	if(ImGui::Combo("animation", &anim_idx, anim_list.data(), anim_list.size())) {
		animator.setAnim(&animator.src_md->animations[anim_idx]);
	}
	ImGui::Text("#tracks in anim : %d", animator.cur_anim->tracks.size());
}

void LimGui::ModelViewEditor(ModelView& md)
{
	if( picked_md_view != &md ) {
		picked_md_view = &md;
		picked_md_data = nullptr;
		picked_rd_node = nullptr;
		picked_bone_node = nullptr;
	}
	ImGui::Begin(md_view_editor_win_name.c_str());


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Render tree") ) {
		drawHierarchy(md.root);
	}
	
	if( md.own_animator ) {
		if( ImGui::CollapsingHeader("Bone tree") ) {
			ImGui::Text("depth of bone root : %d", md.src_md->depth_of_bone_root_in_rdtree);
			drawHierarchy(md.own_animator->bones, 0);
		}
		
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if( ImGui::CollapsingHeader("Animator") ) {
			drawAnimator(*md.own_animator);
		}
	}
	if( ImGui::CollapsingHeader("Etc") ) {
		ImGui::Text("prev tf(Todo)");
		if( ImGui::Button("Open source model data") ) {
			picked_md_data = md.src_md;
		}
	}
	ImGui::End();

	if( picked_rd_node ) {
		RdNodeEditor(*picked_rd_node);
	}
	else if( picked_bone_node ) {
		BoneNodeEditor(*picked_bone_node);
	}
}

BoneNode* LimGui::getPickedBoneNode() {
	return picked_bone_node;
}

RdNode* LimGui::getPickedRenderNode() {
	return picked_rd_node;
}


//
//	ModelDataEditor
//
void LimGui::ModelDataEditor(Model& md)
{
	ImGui::Begin(md_data_editor_win_name.c_str());

	ImGui::Text("name : %s", md.name.c_str());
	ImGui::Text("path : %s", md.path.c_str());
	if( ImGui::TreeNode("boundary") ) {
		ImGui::Text("min : %.2f %.2f %.2f", md.boundary_min.x, md.boundary_min.y, md.boundary_min.z);
		ImGui::Text("max : %.2f %.2f %.2f", md.boundary_max.x, md.boundary_max.y, md.boundary_max.z);
		ImGui::Text("size: %.2f %.2f %.2f", md.boundary_size.x, md.boundary_size.y, md.boundary_size.z);
		ImGui::TreePop();
	}
	if( ImGui::TreeNode("number of") ) {
		ImGui::Text("verteces : %d", md.total_verts);
		ImGui::Text("tris : %d", md.total_tris);
		ImGui::Text("meshes : %d", md.own_meshes.size());
		ImGui::Text("textures : %d", md.own_textures.size());
		ImGui::Text("materials : %d", md.own_materials.size());
		ImGui::TreePop();
	}
	ImGui::End();
}


//
//	RdNodeEditor
//
void LimGui::RdNodeEditor(RdNode& nd)
{
	ImGui::Begin(rd_node_editor_win_name.c_str());

	ImGui::Text("name : %s", nd.name.c_str());
	if( nd.parent ) {
		ImGui::Text("parrent name : %s", nd.parent->name.c_str());
	}
	ImGui::Text("#childs : %d", nd.childs.size());
	ImGui::Checkbox("enabled", &nd.enabled);
	ImGui::Checkbox("is identity mtx", &nd.is_identity_mtx);
	ImGui::Checkbox("is local is global", &nd.is_local_is_global);
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
		nd.updateGlobalTransform();
	}
	LimGui::Mat4(nd.tf.mtx);
	if( nd.ms ) {
		ImGui::TextUnformatted("<Mesh>");
		const Mesh& ms = *nd.ms;
		ImGui::Text("name : %s", ms.name.c_str());
		ImGui::Text("#tris: %d, #verts: %d", ms.nr_tris, ms.nr_verts);
		ImGui::Text("pos:%s,nor:%s,uv:%s,col:%s,tan:%s,bitan:%s,bone_info:%s"
			, boolOX(!ms.poss.empty()), boolOX(!ms.nors.empty()), boolOX(!ms.uvs.empty())
			, boolOX(!ms.cols.empty()), boolOX(!ms.tangents.empty())
			, boolOX(!ms.bitangents.empty()), boolOX(!ms.bone_infos.empty()));

		ImGui::TextUnformatted("<Material>");
		Material& mat = *(Material*)nd.mat; // warning : rid const
		ImGui::Text("name : %s", mat.name.c_str());
		ImGui::SliderFloat("shininess", &mat.Shininess, 0.5f, 300);
		ImGui::SliderFloat("roughness", &mat.Roughness, 0.015f, 1);
		ImGui::SliderFloat("metalness", &mat.Metalness, 0.000f, 1);
		ImGui::ColorEdit3("ambient color", &mat.AmbientColor[0]);

	}
	
	ImGui::End();
}

//
//	BoneNodeEditor
//
void LimGui::BoneNodeEditor(BoneNode& nd)
{
	ImGui::Begin(bone_node_editor_win_name.c_str());

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
		picked_md_view->own_animator->updateMtxBones();
	}
	LimGui::Mat4(nd.tf.mtx);
	ImGui::Text("bone_idx : %d", nd.idx_weighted_bone);
	if( nd.idx_weighted_bone>=0 ) {
		LimGui::Mat4(picked_md_view->src_md->weighted_bone_offsets[nd.idx_weighted_bone]);
	}

	ImGui::End();
}







//
//	LightDirectionalEditor
//
void LimGui::LightDirectionalEditor(LightDirectional& lit)
{
	const static float lit_theta_spd = 70 * 0.001f;
	const static float lit_phi_spd = 360 * 0.001f;
	const static float lit_dist_spd = 45.f * 0.001f;
	static bool is_light_draged = false;
	static bool is_draw_shadow_map_view = false;
	ImGui::Begin(light_editor_window_name.c_str());
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
		lim::Viewport& vp = AssetLib::get().texture_viewer; // todo
		vp.getFb().bind();
		drawTexToQuad(lit.shadow->map.getRenderedTexId(), 2.2f, 0.f, 1.f);
		vp.getFb().unbind();
		LimGui::Viewport(vp);
	}
	ImGui::End();
}

void LimGui::LightSpotEditor(LightSpot& lit)
{
	ImGui::Begin(light_editor_window_name.c_str());
	ImGui::End();
}

void LimGui::LightOmniEditor(LightOmni& lit)
{
	ImGui::Begin(light_editor_window_name.c_str());
	ImGui::End();
}

void LimGui::IBLightEditor(IBLight& lit)
{
	ImGui::Begin(light_editor_window_name.c_str());
	ImGui::End();
}






//
//	SceneEditor
//
void LimGui::SceneEditor(Scene& scene)
{
	ImGui::Begin(scene_editor_window_name.c_str());

	int i;
	if( ImGui::CollapsingHeader("Model views") ) {
		i=0;
		for( auto& md : scene.own_mds ) {
			ImGui::PushID(i++);
			ImGui::Bullet();
            if( ImGui::Selectable(md->root.name.c_str(), md == picked_md_view) ) {
				picked_md_view = md.raw;
				picked_md_data = nullptr;
				picked_rd_node = nullptr;
				picked_bone_node = nullptr;
			};
			ImGui::PopID();
        }
    }
    if( ImGui::CollapsingHeader("Lights") ) {
		i=0;
		for( auto& lit : scene.own_dir_lits ) {
			ImGui::PushID(i++);
			ImGui::Bullet();
            if( ImGui::Selectable(lit->name.c_str(), lit == picked_dir_lit) ) {
				picked_dir_lit = lit.raw;
			}
			ImGui::PopID();
        }
		i=0;
		for( auto& lit : scene.own_spot_lits ) {
			ImGui::PushID(i++);
			ImGui::Bullet();
            if( ImGui::Selectable(lit->name.c_str(), lit == picked_spot_lit) ) {
				picked_spot_lit = lit.raw;
			}
			ImGui::PopID();
        }
		i=0;
		for( auto& lit : scene.own_omni_lits ) {
			ImGui::PushID(i++);
			ImGui::Bullet();
            if( ImGui::Selectable(lit->name.c_str(), lit == picked_omni_lit) ) {
				picked_omni_lit = lit.raw;
			}
			ImGui::PopID();
        }
    }
    if( ImGui::CollapsingHeader("Model datas") ) {
		i=0;
        for( Model* md : scene.src_mds ) {
			ImGui::PushID(i++);
			ImGui::Bullet();
            if( ImGui::Selectable(md->name.c_str(), md == picked_md_data) ) {
				picked_md_view = md;
				picked_md_data = md;
				picked_rd_node = nullptr;
				picked_bone_node = nullptr;
			};
			ImGui::PopID();
        }
    }

	ImGui::End();

	if( picked_md_view ) {
		ModelViewEditor(*picked_md_view);
	}
	if( picked_md_data ) {
		ModelDataEditor(*picked_md_data);
	}

	if( picked_dir_lit ) {
		LightDirectionalEditor(*picked_dir_lit);
	}
	else if( picked_spot_lit ) {
		LightSpotEditor(*picked_spot_lit);
	}
	else if( picked_omni_lit ) {
		LightOmniEditor(*picked_omni_lit);
	}
	else if( picked_ib_light ) {
		IBLightEditor(*picked_ib_light);
	}
}






//
//	Viewport
//

namespace {
	std::function<void(Viewport*)> guizmo_hook = nullptr;
}

void LimGui::Viewport(lim::Viewport& vp)
{
	const IFramebuffer& fb = vp.getFb();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
	ImGui::SetNextWindowSize(
		{(float)fb.width, (float)fb.height+ImGui::GetFrameHeight()},
		(vp.window_mode==Viewport::WM_FIXED_SIZE) ? ImGuiCond_Always : ImGuiCond_Once
	);
	ImGuiWindowFlags vpWinFlag = ImGuiWindowFlags_NoCollapse;
	if( vp.window_mode==Viewport::WM_FIXED_RATIO ) {
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
			lim::Viewport& ivp = *(lim::Viewport*)(data->UserData);

			float frameHeight = ImGui::GetFrameHeight();
			float aspectedWidth = ivp.getAspect() * (data->DesiredSize.y - frameHeight);
			float aspectedHeight = data->DesiredSize.x / ivp.getAspect() + frameHeight;

			if( data->DesiredSize.x <= aspectedWidth )
				data->DesiredSize.x = aspectedWidth;
			else
				data->DesiredSize.y = aspectedHeight;
		}, (void*)&vp);
		vpWinFlag |= ImGuiWindowFlags_NoDocking;
	}
	else if( vp.window_mode==Viewport::WM_FIXED_SIZE ) {
		vpWinFlag |= ImGuiWindowFlags_NoResize;
	}
	ImGui::Begin(vp.name.c_str(), &vp.is_opened, vpWinFlag);
	
	// for ignore draging
	bool isHoveredOnTitle = ImGui::IsItemHovered(); // when move window
	bool isWindowActivated = ImGui::IsItemActive(); // when resize window

	// update size
	auto contentSize = ImGui::GetContentRegionAvail();
	if( fb.width!=contentSize.x || fb.height !=contentSize.y ) {
		if(contentSize.x>10&&contentSize.y>10) // This is sometimes negative
			vp.resize(contentSize.x, contentSize.y);
	}

	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
	GLuint texID = fb.getRenderedTexId();
	if( texID!=0 ) {
		ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)fb.width, (float)fb.height}, ImVec2{0, 1}, ImVec2{1, 0});
	}

	vp.is_focused = ImGui::IsWindowFocused();
	vp.is_hovered = ImGui::IsItemHovered();
	vp.is_dragged = isWindowActivated && !isHoveredOnTitle && ImGui::IsMouseDown(0);
	vp.is_dragged |= (vp.is_hovered||vp.is_focused)&&(ImGui::IsMouseDown(1)||ImGui::IsMouseDown(2));
	vp.is_appearing = ImGui::IsWindowAppearing();

	vp.prev_mouse_pos = vp.mouse_pos;
	ImVec2 imMousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImVec2(0, ImGui::GetFrameHeight());
	vp.mouse_pos = {imMousePos.x, imMousePos.y};
	vp.mouse_uv_pos = {imMousePos.x/contentSize.x, imMousePos.y/contentSize.y};
	vp.mouse_off = vp.mouse_pos - vp.prev_mouse_pos;
	vp.prev_mouse_pos = vp.mouse_pos;

	ImGuiIO io = ImGui::GetIO();
	vp.is_scrolled =  vp.is_hovered&&(io.MouseWheel||io.MouseWheelH);
	vp.scroll_off = {io.MouseWheelH, io.MouseWheel};
	
	if( guizmo_hook ) {
		const auto& pos = ImGui::GetItemRectMin();
		const auto& size = ImGui::GetItemRectSize();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
		guizmo_hook(&vp);
		vp.is_dragged &= !ImGuizmo::IsUsingAny();
	}
	if( vp.is_dragged ) ImGui::SetMouseCursor(7);

	ImGui::End();
	ImGui::PopStyleVar();

	for( auto& cb : vp.update_callbacks ) {
		cb(ImGui::GetIO().DeltaTime);
	}
}


void LimGui::ViewportWithGuizmo(
	ViewportWithCamera& vp
	, std::function<void(lim::Viewport*)> guizmoHook
)
{
	guizmo_hook = guizmoHook;
	LimGui::Viewport(vp);
	guizmo_hook = nullptr;
}

namespace {
	std::function<void(lim::Viewport*)> inner_scene_guizmo_hook = nullptr;
	ImGuizmo::MODE gzmo_space = ImGuizmo::MODE::LOCAL;
	constexpr ImGuizmo::OPERATION gzmo_edit_modes[] = { 
		(ImGuizmo::OPERATION)0, ImGuizmo::TRANSLATE, 
		ImGuizmo::SCALE, ImGuizmo::ROTATE, ImGuizmo::UNIVERSAL 
	};
	int selected_edit_mode_idx = 0;
}
static void sceneGuizmoHook(lim::Viewport* vp)
{
	const auto& pos = ImGui::GetItemRectMin();
		const auto& size = ImGui::GetItemRectSize();
	Camera& cam = ((lim::ViewportWithCamera*)vp)->camera;
	if( inner_scene_guizmo_hook ) {
		inner_scene_guizmo_hook(vp);
	}
    ImGuizmo::SetOrthographic(false);
	ImGuizmo::ViewManipulate( glm::value_ptr(cam.mtx_View), 8.0f, ImVec2{pos.x+size.x-128, pos.y}, ImVec2{128, 128}, (ImU32)0x10101010 );
	if( picked_rd_node ) {
		if( ImGuizmo::Manipulate( glm::value_ptr(cam.mtx_View), glm::value_ptr(cam.mtx_Proj)
                            , gzmo_edit_modes[selected_edit_mode_idx], gzmo_space
							, glm::value_ptr(picked_rd_node->mtx_global)
                            , nullptr, nullptr, nullptr) )
		{
			picked_rd_node->tf.mtx = picked_rd_node->mtx_global;
			picked_rd_node->tf.decomposeMtx();
			picked_rd_node->updateGlobalTransform();
		}
	}
}

void LimGui::ViewportWithSceneGuizmo(
	lim::ViewportWithCamera& vp
    , std::function<void(lim::Viewport*)> guizmoHook
)
{
	inner_scene_guizmo_hook = guizmoHook;
	LimGui::ViewportWithGuizmo(vp, sceneGuizmoHook);
	inner_scene_guizmo_hook = nullptr;
}


