#include "sdf_bridge.h"
#include "sdf_global.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/color_space.hpp>
#include <limbrary/limgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vector>
#include <fstream>
#include <stack>

#if defined(_WIN32)
// #include <commdlg.h>
#endif

using namespace lim;

// todo: 1.uniform block   2.look up table texture
/************* glsl data **************/
// optimize options
int nr_march_steps = 200;
float far_distance = 80.0;
float hit_threshold = 0.0001;
float diff_for_normal = 0.0001;

// mat
glm::vec3 base_colors[MAX_MATS]; // linear space
float roughnesses[MAX_MATS];
float metalnesses[MAX_MATS];
glm::vec3 sky_color = {0.01,0.001,0.3};

// obj
int nr_objs = 0;
glm::mat4 transforms[MAX_OBJS]; // inversed
float scaling_factors[MAX_OBJS];
int mat_idxs[MAX_OBJS];
int op_types[MAX_OBJS];   // Todo: runtime edit shader
int prim_types[MAX_OBJS]; // Todo: runtime edit shader
int prim_idxs[MAX_OBJS];
float blendnesses[MAX_OBJS];
float roundnesses[MAX_OBJS];

// each prim ...
glm::vec2 donuts[MAX_PRIMS];
float capsules[MAX_PRIMS];
/*******************************************/



/***************** application data ****************/
int nr_groups = 0;
int nr_mats = 0;

ObjNode root("root", PT_GROUP, nullptr);
ObjNode* selected_obj = nullptr;
int nr_prims[nr_prim_types] = {0,};

SdfMaterial materials[MAX_MATS]; // SRGB space
const char* mat_names[MAX_MATS]; // cache for gui
int selected_mat_idx = 0;

int                 selected_edit_mode_idx = 1;
ImGuizmo::MODE 		gzmo_space     = ImGuizmo::MODE::LOCAL;

std::string model_name = "Untitled";
CameraController* camera = nullptr;
Light* light = nullptr;
/*****************************************************/


ObjNode::ObjNode(std::string_view _name, PrimitiveType primType, ObjNode* p) {
    name = _name;
    prim_type = primType;
    prim_idx = nr_prims[prim_type]++;
    parent = p;
    if( prim_type == PT_GROUP ) {
        return;
    }
    obj_idx = nr_objs++;
    if( nr_objs==MAX_OBJS ) {
        log::err("maximum of primitives\n");
        exit(1);
    }
    updateGlsl();
    composeTransform();
}
ObjNode::~ObjNode() {
    for(ObjNode* child: children) {
        delete child;
    }
}
void ObjNode::updateGlsl() {
    mat_idxs[obj_idx] = mat_idx;
    prim_types[obj_idx] = prim_type;
    prim_idxs[obj_idx] = prim_idx;
    op_types[obj_idx] = op_group*2 + op_spec;
    blendnesses[obj_idx] = blendness;
    roundnesses[obj_idx] = roundness;
}
float ObjNode::getScaleFactor() {
    float parentFactor = (parent)?parent->getScaleFactor():1.f;
    return parentFactor * glm::min(scale.x, glm::min(scale.y, scale.z));
}
void ObjNode::updateTransformWithParent() {
    if(parent) {
        transform = parent->transform * my_transform;
    } else {
        transform = my_transform;
    }
    if(prim_type==PT_GROUP) {
        for(ObjNode* child : children) {
            child->updateTransformWithParent();
        }
    } else {
        transforms[obj_idx] = glm::inverse(transform);
        scaling_factors[obj_idx] = getScaleFactor();
    }
}
// local info to world transform
void ObjNode::composeTransform() {
    // 인스펙터에서 로컬로 조작한다.
    ImGuizmo::RecomposeMatrixFromComponents(
                glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale)
                ,glm::value_ptr(my_transform));
    updateTransformWithParent();
}
// world transform to local info
void ObjNode::decomposeTransform() {
    my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;

    ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(my_transform)
                ,glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale));

    updateTransformWithParent();
}


SdfMaterial::SdfMaterial(std::string_view _name) {
    name = _name;
    idx = nr_mats++;
}
void SdfMaterial::updateGlsl() {
    mat_names[idx] = name.c_str();
    mat_names[idx+1] = "Add New Material";
    base_colors[idx] = glm::convertSRGBToLinear(base_color);
    roughnesses[idx] = roughness;
    metalnesses[idx] = metalness;
}

static void addMaterial() {
    int idx = nr_mats++;
    SdfMaterial& mat = materials[idx];
    selected_mat_idx = idx;

    mat.idx = idx;
    mat.name = fmtStrToBuf("Material_%d", idx);
    mat.updateGlsl();
}

static void makeSpaceInGlslObjData(int pos, int len) {
    int nr_rights = nr_objs - pos;
    memcpy((float*)&transforms[pos+len], (float*)&transforms[pos], sizeof(glm::mat4)*nr_rights);
    memcpy((int*)&mat_idxs[pos+len], (int*)&mat_idxs[pos], sizeof(int)*nr_rights);
    memcpy((int*)&prim_types[pos+len], (int*)&prim_types[pos], sizeof(int)*nr_rights);
    memcpy((int*)&prim_idxs[pos+len], (int*)&prim_idxs[pos], sizeof(int)*nr_rights);
    memcpy((int*)&op_types[pos+len], (int*)&op_types[pos], sizeof(int)*nr_rights);
    memcpy((float*)&blendnesses[pos+len], (float*)&blendnesses[pos], sizeof(float)*nr_rights);
    memcpy((float*)&roundnesses[pos+len], (float*)&roundnesses[pos], sizeof(float)*nr_rights);
}

void lim::sdf::init(CameraController* cam, Light* lit) 
{
    camera = cam;
    light = lit;

    addMaterial();

    root.name = "root";
    root.prim_type = PT_GROUP;


    root.children.push_back(new ObjNode(fmtStrToBuf("%s_%d", prim_type_names[PT_BOX], nr_prims[PT_BOX]), PT_BOX, &root));
    selected_obj = root.children.back();
}
void lim::sdf::deinit() 
{
    root = ObjNode();
    nr_objs = 0;
    nr_groups = 0;
    nr_mats = 0;
    selected_obj = nullptr;
    for(int i=0;i<nr_prim_types;i++) {
        nr_prims[i] = 0;
    }
}
void lim::sdf::bindSdfData(const Program& prog) 
{
    prog.setUniform("camera_Aspect", camera->aspect);
	prog.setUniform("camera_Fovy", camera->fovy);
	prog.setUniform("camera_Pos", camera->position);
	prog.setUniform("camera_Pivot", camera->pivot);
    prog.setUniform("light_Pos", light->position);
    prog.setUniform("light_Int", light->intensity);

    prog.setUniform("nr_march_steps", nr_march_steps);
    prog.setUniform("far_distance", far_distance);
    prog.setUniform("hit_threshold", hit_threshold);
    prog.setUniform("diff_for_normal", diff_for_normal);

    prog.setUniform("base_colors", MAX_OBJS, base_colors);
    prog.setUniform("roughnesses", MAX_MATS, roughnesses);
    prog.setUniform("metalnesses", MAX_MATS, metalnesses);
    prog.setUniform("sky_color", sky_color);
    
    prog.setUniform("nr_objs", nr_objs);
    prog.setUniform("transforms", MAX_OBJS, transforms);
    prog.setUniform("scaling_factors", MAX_OBJS, scaling_factors);
    prog.setUniform("mat_idxs", MAX_OBJS, mat_idxs);
    prog.setUniform("prim_types", MAX_OBJS, prim_types);
    prog.setUniform("op_types", MAX_OBJS, op_types);
    prog.setUniform("blendnesses", MAX_OBJS, blendnesses);
    prog.setUniform("roundnesses", MAX_OBJS, roundnesses);

    prog.setUniform("donuts", MAX_PRIMS, donuts);
    prog.setUniform("capsules", MAX_PRIMS, capsules);
}

static void drawObjTree(ObjNode& obj) 
{
    bool isGroup = obj.prim_type==PT_GROUP;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if( isGroup) {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_DefaultOpen;
    }
    else {
        flags |= ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
    }
    if(selected_obj == &obj) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isOpen = ImGui::TreeNodeEx(&obj, flags, "%s", obj.name.c_str());

    if( selected_obj!=&obj &&(ImGui::IsItemClicked(0)||ImGui::IsItemClicked(1))) {
        selected_obj = &obj;
        selected_mat_idx = obj.mat_idx;
    }

    if( ImGui::BeginPopupContextItem() ) {
        if( obj.prim_type==PT_GROUP && ImGui::BeginMenu("Add") ) {
            for(int i=0; i<nr_prim_types; i++) {
                if( ImGui::MenuItem(prim_type_names[i], fmtStrToBuf("Ctrl+%d",i), false, true) ) {
                    obj.children.push_back(new ObjNode(fmtStrToBuf("%s_%d", prim_type_names[i], nr_prims[i]), (PrimitiveType)i, &obj));
                    selected_obj = obj.children.back();
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
        ImGui::SetDragDropPayload("DND_SCENE_CELL", &obj, sizeof(ObjNode));

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
        for( ObjNode* child: obj.children) {
            drawObjTree(*child);
        }
        ImGui::TreePop();
    }
}

extern void exportMesh(std::string_view dir, std::string_view modelName, int sampleRate);

void lim::sdf::drawImGui() 
{
    /* menu bar */
    if( ImGui::BeginMainMenuBar() )
    {
        static bool isJsonExporterOpened = false;
        static bool isMeshExporterOpened = false;
        static constexpr ImVec2 popupSize = {300, 300};
        static constexpr ImVec2 doneSize = {200, 100};

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGuiWindowFlags popupWindowFlags = ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize;

        if( ImGui::MenuItem("Export Json") ) {
            ImGui::OpenPopup("JsonExporter");
            isJsonExporterOpened = true;
        }
        ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(popupSize);
        if( ImGui::BeginPopupModal("JsonExporter", &isJsonExporterOpened, popupWindowFlags) ) {
            ImGui::InputText("File Name", &model_name, ImGuiInputTextFlags_AutoSelectAll);

            ImGui::Text("program_dir/exports/%s.json", model_name.c_str());
            
            std::filesystem::path path = fmtStrToBuf("exports/%s.json", model_name.c_str());

            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
            if( ImGui::Button("Export", {-1,0}) ) {
                exportJson(path);
                ImGui::OpenPopup("Done");
            }
            ImGui::SetNextWindowSize(doneSize);
            ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
            if( ImGui::BeginPopupModal("Done",NULL,popupWindowFlags) ) {
                ImGui::Text("%s", path.c_str());
                
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
                if( ImGui::Button("Close", {-1,0}) ) {
                    ImGui::CloseCurrentPopup();
                    isJsonExporterOpened = false;
                }
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }


        if( ImGui::MenuItem("Export Mesh") ) {
            ImGui::OpenPopup("MeshExporter");
            isMeshExporterOpened = true;
        }
        ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(popupSize);
        if( ImGui::BeginPopupModal("MeshExporter", &isMeshExporterOpened, popupWindowFlags) ) {
            static int sampleRate = 200;
            std::string exportPath = fmtStrToBuf("program_dir/exports/%s/%s.obj", model_name.c_str(), model_name.c_str());
            ImGui::InputText("File Name", &model_name, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::TextUnformatted(exportPath.c_str());
        
            ImGui::SliderInt("Sample", &sampleRate, 50, 1000);
            ImGui::Text("%dx%dx%d", sampleRate, sampleRate, sampleRate);


            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
            if( ImGui::Button("Export", {-1,0}) ) {
                exportMesh("exports", model_name.c_str(), sampleRate);
                ImGui::OpenPopup("Done");
            }
            ImGui::SetNextWindowSize(doneSize);
            ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
            if( ImGui::BeginPopupModal("Done",NULL,popupWindowFlags) ) {
                ImGui::TextUnformatted(exportPath.c_str());
                
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
                if( ImGui::Button("Close", {-1,0}) ) {
                    ImGui::CloseCurrentPopup();
                    isMeshExporterOpened = false;
                }
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }

        if( ImGui::MenuItem("Import") ) {
            // Todo: 윈도우 파일 오픈 다이얼로그
        }


        if( ImGui::BeginMenu("Help") ) {
            ImGui::Text("drag and drop json file to import model");
            ImGui::Text("you can use my model viewer if you want viewing exported mesh model");
            ImGui::EndMenu();
        }
        if( ImGui::BeginMenu("Contact") ) {
            ImGui::Text("hi my name is imdongye. nice to meet you");
            ImGui::Text("my email is imdongye@naver.com");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
	
   

    /* Scene Hierarchy */
    {
        ImGui::Begin("Scene##sdf");

        drawObjTree(root);

        // ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*2.3f);
        // ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
        if( ImGui::Button("Add", {-1,0}) ) {
            ImGui::OpenPopup("AddObj");
        }
        if( ImGui::BeginPopup("AddObj") ) {
            while( selected_obj->prim_type!=PT_GROUP ) {
                selected_obj = selected_obj->parent;
            }
            for(int i=0; i<nr_prim_types; i++) {
                if( ImGui::Selectable(prim_type_names[i]) ) {
                    selected_obj->children.push_back(new ObjNode(fmtStrToBuf("%s_%d", prim_type_names[i], nr_prims[i]), (PrimitiveType)i, selected_obj));
                    selected_obj = selected_obj->children.back();
                }
            }
            ImGui::EndPopup();
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
        ImGui::InputText("Name", &obj.name, ImGuiInputTextFlags_AutoSelectAll);

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
        if( obj.prim_type==PT_GROUP ) {
            LimGui::CheckBox3("Mirror", glm::value_ptr(obj.mirror));
        }
        else {
            if( ImGui::Combo("OperGroup", (int*)&obj.op_group, prim_oper_group_names, nr_prim_oper_groups) ) {
                op_types[obj.obj_idx] = obj.op_group*2 + obj.op_spec;
            }
            if( ImGui::Combo("OperSpec", (int*)&obj.op_spec, prim_oper_spec_names, nr_prim_oper_specs) ) {
                op_types[obj.obj_idx] = obj.op_group*2 + obj.op_spec;
            }
            if( ImGui::SliderFloat("Blendness", &obj.blendness, 0.f, 1.f) ) {
                blendnesses[obj.obj_idx] = obj.blendness;
            }
            if( ImGui::SliderFloat("Roundness", &obj.roundness, 0.f, 1.f) ) {
                roundnesses[obj.obj_idx] = obj.roundness;
            }
            if( ImGui::Combo("Material", &obj.mat_idx, mat_names, nr_mats+1) ) {
                selected_mat_idx = obj.mat_idx;
                mat_idxs[obj.obj_idx] = obj.mat_idx;
                if( obj.mat_idx==nr_mats ) {
                    addMaterial();
                }
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
        ImGui::SliderFloat("Hit Threshold", &hit_threshold, 0.00001, 0.001, "%.5f");
        ImGui::SliderFloat("Diff for Normal", &diff_for_normal, 0.00001, 0.001, "%.5f");
        ImGui::Separator();

        ImGui::Text("<Light>");
        static float litTheta = 90.f-glm::degrees(glm::acos(glm::dot(glm::normalize(light->position), glm::normalize(glm::vec3(light->position.x, 0, light->position.z)))));
        const static float litThetaSpd = 70 * 0.001;
        static float litPhi = 90.f-glm::degrees(glm::atan(light->position.x,-light->position.z));
        const static float litPhiSpd = 360 * 0.001;
        static float litDist = glm::length(light->position);
        const static float litDistSpd = 45.f * 0.001;
        bool isLightDraged = false;
        isLightDraged |= ImGui::DragFloat("Yaw", &litPhi, litPhiSpd, -FLT_MAX, +FLT_MAX, "%.3f");
        isLightDraged |= ImGui::DragFloat("Pitch", &litTheta, litThetaSpd, 0, 80, "%.3f");
        isLightDraged |= ImGui::DragFloat("Distance", &litDist, litDistSpd, 5.f, 20.f, "%.3f");
        if( isLightDraged ) {
            light->setRotate(litTheta, glm::fract(litPhi/360.f)*360.f, litDist);
        }
        ImGui::Text("Pos: %.1f %.1f %.1f", light->position.x, light->position.y, light->position.z);
        ImGui::SliderFloat("Intencity", &light->intensity, 0.0f, 200.f, "%.1f");
        ImGui::Separator();
        ImGui::Text("<Camera>");
        static int viewingMode = 0;
        static const char* viewingModeNames[] = {"Free", "Pivoted"};
        if( ImGui::Combo("Mode",  &viewingMode, viewingModeNames, 3) ) {
            camera->setViewMode((CameraController::VIEWING_MODE)viewingMode);
        }
        
        ImGui::PopItemWidth();
        ImGui::End();
    }
    /* Mat List*/
    {
        ImGui::Begin("Materials##sdf");
        
        for( int i=0; i<nr_mats; i++ ) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
            if( selected_mat_idx == i ) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            SdfMaterial& mat = materials[i];
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
            addMaterial();
        }
        ImGui::End();
    }
    /* Mat Editor */
    {
        ImGui::Begin("Mat Editor##sdf");
        SdfMaterial& mat = materials[selected_mat_idx];
        ImGui::InputText("Name", &mat.name, ImGuiInputTextFlags_AutoSelectAll);
        if( ImGui::SliderFloat("Roughness", &mat.roughness, 0.001, 1.0) ) {
            roughnesses[selected_mat_idx] = mat.roughness;
        }
        if( ImGui::SliderFloat("Metalness", &mat.metalness, 0.001, 1.0) ) {
            metalnesses[selected_mat_idx] = mat.metalness;
        }
        ImGui::TextUnformatted("Base Color");
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        float w = glm::min(280.f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.y);
        ImGui::SetNextItemWidth(w);
        if( ImGui::ColorPicker3("##Base Color", glm::value_ptr(mat.base_color), flags) ) {
            base_colors[selected_mat_idx] = glm::convertSRGBToLinear(mat.base_color);
        }
        ImGui::End();
    }
}
void lim::sdf::drawGuizmo(const Viewport& vp) {
    Camera& cam = *camera;
    const auto& pos = ImGui::GetItemRectMin();
    const auto& size = ImGui::GetItemRectSize();
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    ImGuizmo::SetOrthographic(false);
    if(selected_edit_mode_idx>0) {
        ImGuizmo::Manipulate( glm::value_ptr(cam.view_mat), glm::value_ptr(cam.proj_mat)
                            , gzmo_edit_modes[selected_edit_mode_idx], gzmo_space, glm::value_ptr(selected_obj->transform)
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
        ImVec2 windowPos = {workPos.x+PAD, workPos.y+PAD+PAD*2.3f};
        
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        //ImGui::SetNextWindowSizeConstraints(ImVec2(30.f, 120.f));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {0.5f, 0.5f});
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {2.f, 2.f});
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {0.5f, 0.7f});

        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID); // 뷰포트가 안되도록
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        if( ImGui::Begin("edit mode selector", nullptr, window_flags) )
        {
            static const char* editModeStrs[] = {  u8"\uE820", u8"\uE806", u8"\uE807", u8"\uE811", u8"\uE805" };
            for(int i=0; i<5; i++) {
                if( ImGui::Selectable(editModeStrs[i], selected_edit_mode_idx==i, 0, {30, 30}) ) {
                    selected_edit_mode_idx = i;
                }
            }
            ImGui::End();
        }
        if( selected_edit_mode_idx!=0 && selected_edit_mode_idx!=2 ) {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID); // 뷰포트가 안되도록
            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            ImGui::SetNextWindowPos({windowPos.x, windowPos.y+180}, ImGuiCond_Always);
            if( ImGui::Begin("edit space selector", nullptr, window_flags) )
            {
                if( ImGui::Selectable(u8"\uE832", gzmo_space==ImGuizmo::WORLD, 0, {30, 30}) ) {
                    gzmo_space = ImGuizmo::WORLD;
                }
                if( ImGui::Selectable(u8"\uE808", gzmo_space==ImGuizmo::LOCAL, 0, {30, 30}) ) {
                    gzmo_space = ImGuizmo::LOCAL;
                }
                ImGui::End();
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }
    /* short cut */
    {
        if( ImGui::IsKeyPressed(ImGuiKey_1, false) ) {
			selected_edit_mode_idx = 0;
		}
        if( ImGui::IsKeyPressed(ImGuiKey_2, false) ) {
			selected_edit_mode_idx = 1;
		}
        if( ImGui::IsKeyPressed(ImGuiKey_3, false) ) {
			selected_edit_mode_idx = 2;
		}
        if( ImGui::IsKeyPressed(ImGuiKey_4, false) ) {
			selected_edit_mode_idx = 3;
		}
        if( ImGui::IsKeyPressed(ImGuiKey_5, false) ) {
			selected_edit_mode_idx = 4;
		}
    }
}