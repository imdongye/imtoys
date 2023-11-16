#include "sdf_lib.h"
#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imguizmo/ImGuizmo.h>
#include <limbrary/limgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vector>

namespace {
    constexpr int nr_oper_types = 3;
    const char* prim_oper_names[nr_oper_types] = {
        "Addition", "Subtitution", "Intersection"
    };

    constexpr int nr_prim_types = 5;
    const char* prim_type_names[nr_prim_types] = {
        "Group", "Sphere", "Box", "Pipe", "Donut"
    };
}

/* application data */
namespace {
    lim::sdf::ObjNode root;
    lim::sdf::ObjNode* selected_obj = nullptr;
    std::vector<lim::sdf::Material> materials;
    int selected_mat_idx = 0;
    int nr_prims[nr_prim_types] = {0,};

    ImGuizmo::OPERATION gzmo_edit_mode = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE 		gzmo_space = ImGuizmo::MODE::LOCAL;
};

/* glsl data */
// todo: 1.uniform block   2.look up table texture
namespace {
    constexpr int MAX_MATS = 32;
    constexpr int MAX_OBJS = 32;
    constexpr int MAX_PRIMS = 32;

    // optimize options
    int nr_march_steps = 100;
    float far_distance = 100.0;
    float hit_threshold = 0.01;
    float diff_for_normal = 0.01;

    // mat
    glm::vec3 base_colors[MAX_MATS];
    float roughnesses[MAX_MATS];
    float metalnesses[MAX_MATS];

    // obj
    int nr_objs = 0;
    glm::mat4 transforms[2*MAX_OBJS];
    int mat_idxs[2*MAX_OBJS];
    int prim_types[2*MAX_OBJS];
    int prim_idxs[2*MAX_OBJS];
    int op_types[2*MAX_OBJS];
    float blendnesses[2*MAX_OBJS];
    float roundnesses[2*MAX_OBJS];

    // each prim ...
    float donuts[MAX_PRIMS];
    float cylinder[MAX_PRIMS];
}



lim::sdf::ObjNode::ObjNode(PrimitiveType primType, const ObjNode* p)
    : prim_type(primType), parent(p)
{
    name = fmtStrToBuf("%s_%d", prim_type_names[primType], nr_prims[primType]);
    prim_idx = nr_prims[prim_type]++;

    if(prim_type == PT_GROUP) {
        return;
    }
    
    obj_idx = nr_objs++;
    if(nr_objs==MAX_OBJS) {
        exit(1);
    }

    transforms[obj_idx] = glm::inverse(root.transform);
    mat_idxs[obj_idx] = mat_idx;
    prim_types[obj_idx] = prim_type;
    prim_idxs[obj_idx] = prim_idx;
    op_types[obj_idx] = op_type;
    blendnesses[obj_idx] = blendness;
    roundnesses[obj_idx] = roundness;

    composeTransform();
}
void lim::sdf::ObjNode::addChild(PrimitiveType primType)
{
    children.emplace_back(primType, this);
}
void lim::sdf::ObjNode::updateTransformWithParent() {
    if(parent) {
        transform = parent->transform * my_transform;
    }
    if(obj_idx<0) {
        for(ObjNode& child : children) {
            child.updateTransformWithParent();
        }
    }
    else {
        transforms[obj_idx] = glm::inverse(transform);
    }
}
void lim::sdf::ObjNode::composeTransform() {
    ImGuizmo::RecomposeMatrixFromComponents(
                glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale)
                ,glm::value_ptr(transform));
    
    my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;
    updateTransformWithParent();
}
void lim::sdf::ObjNode::decomposeTransform() {
    my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;

    ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(transform)
                ,glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale));

    updateTransformWithParent();
}

void lim::sdf::makeSpaceInGlslObjData(int pos, int len) {
    int nr_rights = nr_objs - pos;
    memcpy((float*)&transforms[pos+len], (float*)&transforms[pos], sizeof(glm::mat4)*nr_rights);
    memcpy((int*)&mat_idxs[pos+len], (int*)&mat_idxs[pos], sizeof(int)*nr_rights);
    memcpy((int*)&prim_types[pos+len], (int*)&prim_types[pos], sizeof(int)*nr_rights);
    memcpy((int*)&prim_idxs[pos+len], (int*)&prim_idxs[pos], sizeof(int)*nr_rights);
    memcpy((int*)&op_types[pos+len], (int*)&op_types[pos], sizeof(int)*nr_rights);
    memcpy((float*)&blendnesses[pos+len], (float*)&blendnesses[pos], sizeof(float)*nr_rights);
    memcpy((float*)&roundnesses[pos+len], (float*)&roundnesses[pos], sizeof(float)*nr_rights);
}

void lim::sdf::init() 
{
    materials.push_back({"Material_0", {0.2, 0.13, 0.87}, 1.f, 0.f});
    selected_mat_idx = 0;

    root.name = "root";

    root.addChild(ObjNode::PT_BOX);
    selected_obj = &root.children.back();
}
void lim::sdf::deinit() 
{
    nr_objs = 0;
    selected_obj = nullptr;
    for(int i=0;i<nr_prim_types;i++) {
        nr_prims[i] = 0;
    }
    materials.clear();
}
void lim::sdf::bindSdfData(const Program& prog) 
{
    prog.setUniform("nr_march_steps", nr_march_steps);
    prog.setUniform("far_distance", far_distance);
    prog.setUniform("hit_threshold", hit_threshold);
    prog.setUniform("diff_for_normal", diff_for_normal);

    prog.setUniform("base_colors", MAX_OBJS, base_colors);
    prog.setUniform("roughnesses", MAX_MATS, roughnesses);
    prog.setUniform("metalnesses", MAX_MATS, metalnesses);
    
    prog.setUniform("nr_objs", nr_objs);
    prog.setUniform("transforms", MAX_OBJS, transforms);
    prog.setUniform("mat_idxs", MAX_OBJS, mat_idxs);
    prog.setUniform("prim_types", MAX_OBJS, prim_types);
    prog.setUniform("op_types", MAX_OBJS, op_types);
    prog.setUniform("blendness", MAX_OBJS, blendnesses);
    prog.setUniform("roundness", MAX_OBJS, roundnesses);
}

void lim::sdf::drawObjTree(ObjNode* pObj) 
{
    ObjNode& obj = *pObj;
    bool isGroup = obj.prim_type==ObjNode::PT_GROUP;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if( isGroup) {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_DefaultOpen;
    }
    else {
        flags |= ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
    }
    if(selected_obj == pObj) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isOpen = ImGui::TreeNodeEx(pObj, flags, "%s", obj.name.c_str());

    if( selected_obj!=pObj &&(ImGui::IsItemClicked(0)||ImGui::IsItemClicked(1))) {
        selected_obj = pObj;
    }

    if( ImGui::BeginPopupContextItem() ) {
        if( obj.prim_type==ObjNode::PT_GROUP && ImGui::BeginMenu("Add") ) {
            for(int i=0; i<nr_prim_types; i++) {
                if( ImGui::MenuItem(prim_type_names[i], fmtStrToBuf("Ctrl+%d",i), false, true) ) {
                    obj.addChild((ObjNode::PrimitiveType)i);
                }
            }
            ImGui::EndMenu();
        }
        if( ImGui::MenuItem("Delete", "Backspace", false, true) ) {
            log::pure("delelte\n");
        }
        ImGui::EndPopup();
    }

    if( ImGui::BeginDragDropSource(ImGuiDragDropFlags_None) ) {
        ImGui::SetDragDropPayload("DND_SCENE_CELL", pObj, sizeof(ObjNode));

        ImGui::Text("Move %s", obj.name.c_str());

        ImGui::EndDragDropSource();
    }

    if( ImGui::BeginDragDropTarget() ) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_SCENE_CELL"))
        {
            assert(payload->DataSize == sizeof(ObjNode));

            ObjNode& srcObj = *(ObjNode*)payload->Data;
            log::pure("move to %s\n", obj.name.c_str());
            
            // Todo
            // makeSpaceInGlslObjData()
        }
        ImGui::EndDragDropTarget();
    }

    if( isGroup&&isOpen ) {
        for( ObjNode& child: obj.children) {
            drawObjTree(&child);
        }
        ImGui::TreePop();
    }
}

void lim::sdf::drawImGui() 
{
    /* Scene Hierarchy */
    {
        ImGui::Begin("Scene##sdf");

        drawObjTree(&root);

        // ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*2.3f);
        // ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
        if( ImGui::Button("Add", {-1,0}) ) {

        }
        ImGui::End();
    }
    /* Obj Editor */
    {
        ImGui::Begin("Object##sdf");
        float windowWidth = ImGui::CalcItemWidth();
        if(!selected_obj) {
            ImGui::End();
            return;
        }
        ObjNode& obj = *selected_obj;
        ImGui::InputText("Name", &obj.name);

        static const float slide_pos_spd = 4/500.f;
        static const float slide_scale_spd = 4/500.f;
        static const float slide_rot_spd = 180/500.f;
        if(ImGui::DragFloat3("Position", glm::value_ptr(obj.position), slide_pos_spd, -FLT_MAX, +FLT_MAX)) {
            obj.composeTransform();
        }
        if(ImGui::DragFloat3("Scale", glm::value_ptr(obj.scale), slide_scale_spd, -FLT_MAX, +FLT_MAX)) {
            obj.composeTransform();
        }
        if(ImGui::DragFloat3("Rotate", glm::value_ptr(obj.euler_angles), slide_rot_spd, -FLT_MAX, +FLT_MAX)) {
            obj.composeTransform();
        }
        if( obj.prim_type==ObjNode::PT_GROUP ) {
            // for(int i=0;i<3;i++) {
            // 	ImGui::PushID(i);
            // 	ImGui::Checkbox("",&obj.mirror[i]);
            // 	ImGui::PopID();
            // 	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            // }
            // ImGui::Text("Mirror");
            LimGui::CheckBox3("Mirror", glm::value_ptr(obj.mirror));
        }
        else {
            if( ImGui::Combo("Operator", (int*)&obj.op_type, prim_oper_names, nr_oper_types) ) {
                //gzmo_oper = (ImGuizmo::OPERATION)gzmoOpers[selectecGzmoOperIdx];
            }
            if( ImGui::SliderFloat("Blendness", &obj.blendness, 0.f, 1.f) ) {
            }
            if( ImGui::SliderFloat("Roundness", &obj.roundness, 0.f, 1.f) ) {
            }
        }

        
        ImGui::End();
    }
    /* Settings */
    {
        ImGui::Begin("Settings##sdf");
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        ImGui::SliderInt("# March Steps", &nr_march_steps, 20, 300, "%d");
        ImGui::SliderFloat("Far Distance", &far_distance, 20, 300, "%.0f");
        ImGui::SliderFloat("Hit Threshold", &hit_threshold, 0.0001, 0.1, "%.4f");
        ImGui::SliderFloat("Diff for Normal", &diff_for_normal, 0.0001, 0.1, "%.4f");
        ImGui::PopItemWidth();
        ImGui::End();
    }
    /* Mat List*/
    {
        ImGui::Begin("Materials##sdf");
        
        for( int i=0; i<materials.size(); i++ ) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
            if( selected_mat_idx == i ) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            Material& mat = materials[i];
            if( ImGui::TreeNodeEx("MatList", flags, "%s", mat.name.c_str()) ) {
                if( selected_mat_idx != i &&(ImGui::IsItemClicked(0)||ImGui::IsItemClicked(1))) {
                    selected_mat_idx = i;
                }
            }
        }

        // ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*2.3f);
        // ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
        if( ImGui::Button("Add", {-1,0}) ) {
            selected_mat_idx = materials.size();
            materials.push_back({});
            materials.back().name = fmtStrToBuf("Material_%d", selected_mat_idx);
        }
        ImGui::End();
    }
    /* Mat Editor */
    {
        ImGui::Begin("Mat Editor##sdf");
        Material& mat = materials[selected_mat_idx];
        ImGui::InputText("Name", &mat.name);
        ImGui::SliderFloat("Roughness", &mat.roughness, 0.001, 1.0);
        ImGui::SliderFloat("Metalness", &mat.metalness, 0.001, 1.0);
        ImGui::TextUnformatted("Base Color");
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        float w = glm::min(280.f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.y);
        ImGui::SetNextItemWidth(w);
        ImGui::ColorPicker3("##Base Color", glm::value_ptr(mat.base_color), flags);
        ImGui::End();
    }
}
void lim::sdf::drawGuizmo(const Viewport& vp, Camera& cam) {
    const auto& pos = ImGui::GetItemRectMin();
    const auto& size = ImGui::GetItemRectSize();
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    ImGuizmo::SetOrthographic(false);
    if(gzmo_edit_mode>0) {
        ImGuizmo::Manipulate( glm::value_ptr(cam.view_mat), glm::value_ptr(cam.proj_mat)
                            , gzmo_edit_mode, gzmo_space, glm::value_ptr(selected_obj->transform)
                            , nullptr, nullptr, nullptr);
    }
    
    if( ImGuizmo::IsUsing() ) {
        selected_obj->decomposeTransform();
    }
    // Axis
    ImGuizmo::ViewManipulate( glm::value_ptr(cam.view_mat), 8.0f, ImVec2{pos.x+size.x-128, pos.y}, ImVec2{128, 128}, (ImU32)0x10101010 );

    /* Edit option */
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
        const float PAD = 10.0f;
        ImVec2 workPos = ImGui::GetWindowPos();
        ImVec2 windowPos = {workPos.x+PAD, workPos.y+PAD+PAD*1.8f};
        
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        //ImGui::SetNextWindowSizeConstraints(ImVec2(30.f, 120.f));
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID); // 뷰포트가 안되도록
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if( ImGui::Begin("edit mode selector", nullptr, window_flags) )
        {
            static int selectedEditModeIdx = 1;
            static const char* editModeStrs[] = { "Select", "Position", "Scale", "Rotate", "Transform" };
            static const int   editModes[] = { 0, ImGuizmo::TRANSLATE, ImGuizmo::SCALE, ImGuizmo::ROTATE, ImGuizmo::UNIVERSAL };
            for(int i=0; i<5; i++) {
                if( ImGui::Selectable(editModeStrs[i], selectedEditModeIdx==i, 0, {30, 30}) ) {
                    selectedEditModeIdx = i;
                    gzmo_edit_mode = (ImGuizmo::OPERATION)editModes[i];
                }
            }
        }
        //ImGui::PopStyleVar();
        ImGui::End();
    }
}