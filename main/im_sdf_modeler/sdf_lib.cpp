/*

    2023.12.06 / im dongye

    Todo:
    mirror
    그룹 연산자 하위 전파
    콘, ... 프리미티브 추가
    그림자, AO
    단축키
    원점아니라 화면중간에 생성

*/
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
#include <algorithm>
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
glm::mat4 transforms[MAX_OBJS]; // inversed
float scaling_factors[MAX_OBJS];
int mat_idxs[MAX_OBJS];
int op_types[MAX_OBJS];   // Todo: runtime edit shader
int prim_types[MAX_OBJS]; // Todo: runtime edit shader
int prim_idxs[MAX_OBJS];
float blendnesses[MAX_OBJS];
float roundnesses[MAX_OBJS];

// each prim ...
float donuts[MAX_PRIMS];
float capsules[MAX_PRIMS];

IBLight ib_light;
bool use_IBL = false;


/***************** application data ****************/
int nr_each_prim_types[nr_prim_types] = {0,}; // 이름 만들때만 사용. delete동기화 안됨.
int nr_groups = 0;
std::vector<sdf::Object*> serialized_objs;
sdf::Node* selected_obj = nullptr;
sdf::Material* selected_mat = nullptr;

sdf::Group* root = nullptr;

std::vector<sdf::Material*> materials;
const char* mat_names[MAX_MATS]; // gui를 위한 cache

int selected_edit_mode_idx = 1;
ImGuizmo::MODE gzmo_space = ImGuizmo::MODE::LOCAL;

std::string model_name = "Untitled";
CameraController* camera = nullptr;
Light* light = nullptr;


/********************** global func **********************/
static void addMaterial() {
    selected_mat = new sdf::Material();
    int idx = materials.size();
    materials.push_back(selected_mat);
    selected_mat->name = fmtStrToBuf("Material_%d", idx);
    mat_names[idx] = selected_mat->name.c_str();
    mat_names[idx+1] = "Add New Material";
    selected_mat->idx = idx;
    selected_mat->updateShaderData();
}
static void deleteSelectedMaterial() {
    if(materials.size()<=1 || selected_mat==nullptr) {
        lim::log::err("you can't delete last mat\n");
        return;
    }
    int idx = selected_mat->idx;
    sdf::Material* altMat = (idx+1==materials.size())?materials.back():materials[idx];
    for(sdf::Object* obj: serialized_objs) {
        if(obj->p_mat == selected_mat) {
            obj->p_mat = altMat;
        }
    }
    delete selected_mat;
    materials.erase( materials.begin()+idx );
    for(int i=idx; i<materials.size(); i++) {
        materials[i]->idx = i;
        materials[i]->updateShaderData();
    }
    selected_mat = altMat;
}

static void serializeNode(sdf::Node* node) {
    if(node->is_group) {
        sdf::Group* group = (sdf::Group*)node;
        for(sdf::Node* n: group->children) {
            serializeNode(n);
        }
    }
    else {
        sdf::Object* obj = (sdf::Object*)node;
        int idx = serialized_objs.size();
        obj->idx = idx;
        serialized_objs.push_back(obj);
        obj->updateShaderData();
    }
}

void sdf::serializeModel() {
    serialized_objs.clear();
    serializeNode(root);
}

/********************** sdf_obj.h ***********************/
void sdf::Material::updateShaderData() {
    base_colors[idx] = glm::convertSRGBToLinear(base_color);
    roughnesses[idx] = roughness;
    metalnesses[idx] = metalness;
}
void sdf::Object::updateShaderData() {
    transforms[idx]     = glm::inverse(transform);
    scaling_factors[idx]= getScaleFactor();
    mat_idxs[idx]       = p_mat->idx;
    prim_types[idx]     = prim_type;
    prim_idxs[idx]      = prim_idx;
    op_types[idx]       = op_group*2 + op_spec;
    blendnesses[idx]    = blendness;
    roundnesses[idx]    = roundness;
}

sdf::Node::Node(std::string_view _name, Group* _parent) {
    parent = _parent;
    name = _name;
} 
void sdf::Node::updateTransformWithParent() {
    if(parent) {
        transform = parent->transform * my_transform;
    } else {
        transform = my_transform;
    }
    if(is_group) {
        Group* grp = (Group*)this;
        for(Node* child : grp->children) {
            child->updateTransformWithParent();
        }
    }
    else {
        Object* obj = (Object*)this;
        transforms[obj->idx] = glm::inverse(obj->transform);
        scaling_factors[obj->idx] = obj->getScaleFactor(); // Todo: 미리 캐슁
    }
}
// local info to world transform
void sdf::Node::composeTransform() {
    // 인스펙터에서 로컬로 조작한다.
    ImGuizmo::RecomposeMatrixFromComponents(
                glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale)
                ,glm::value_ptr(my_transform));
    updateTransformWithParent();
}
// world transform to local info
void sdf::Node::decomposeTransform() {
    my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;

    ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(my_transform)
                ,glm::value_ptr(position)
                ,glm::value_ptr(euler_angles)
                ,glm::value_ptr(scale));

    updateTransformWithParent();
}
float sdf::Node::getScaleFactor() {
    float parentFactor = (parent)?parent->getScaleFactor():1.f;
    return parentFactor * glm::min(scale.x, glm::min(scale.y, scale.z));
}

sdf::Group::Group(std::string_view _name, Group* _parent)
    : Node(_name, _parent)
{
    is_group = true;
    nr_groups++;
    children.clear();
    selected_obj = this;
}
sdf::Group::~Group() {
    for(Node* node: children) {
        delete node;
    }
}
void sdf::Group::addGroupToBack() {
    Group* child = new Group(fmtStrToBuf("Group_%d", nr_groups), this);
    child->composeTransform();
    children.push_back((Node*)child);
}
void sdf::Group::addObjectToBack(PrimitiveType pt) {
    const char* cname = fmtStrToBuf("%s_%d", prim_type_names[pt], nr_each_prim_types[pt]);
    Object* child = new Object(cname, this, pt);
    child->composeTransform();
    children.push_back((Node*)child);
}
void sdf::Group::rmChild(sdf::Node* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if(it == children.end())
        return;
    
    if(children.size()==1) {
        selected_obj = this;
    } else if(it+1 == children.end()) {
        selected_obj = *(it-1);
    } else {
        selected_obj = *(it+1);
    }

    children.erase(it);
    delete child;

    serializeModel();
}
void sdf::Group::getOtherChild(sdf::Node* child) {
    auto it = std::find(child->parent->children.begin(), child->parent->children.end(), child);
    child->parent->children.erase(it);

    child->parent = this;
    children.push_back(child);

    serializeModel();
}

void sdf::Node::copyDataTo(Node* dst) {
    dst->name = name+"_copied";
    dst->transform = transform;
    dst->decomposeTransform();
    dst->mirror = mirror;
    dst->op_group = op_group;
    dst->op_spec = op_spec;
    dst->blendness = blendness;
    dst->roundness = roundness;
}

void sdf::Group::copyToGroup(Group* group) {
    const int nrChildren = children.size();
    group->addGroupToBack();
    Group* dst = (Group*)group->children.back();
    copyDataTo(dst);
    
    for(int i=0; i<nrChildren; i++) {
        children[i]->copyToGroup(dst);
    }
}

void sdf::Object::copyToGroup(Group* group) {
    group->addObjectToBack((PrimitiveType)prim_type);
    Object* dst = (Object*)group->children.back();
    copyDataTo(dst);

    dst->p_mat = p_mat;
    dst->prim_idx = prim_idx;
}



sdf::Object::Object(std::string_view _name, Group* _parent, PrimitiveType pt)
    : Node(_name, _parent) 
{
    if( serialized_objs.size()==MAX_OBJS ) {
        log::err("maximum of primitives\n");
        exit(1);
    }
    is_group = false;
    prim_type = pt;
    p_mat = selected_mat;
    prim_idx = nr_each_prim_types[pt]++;
    switch(prim_type) {
    case PT_DONUT:
        donuts[prim_idx] = 1.f;
        break;
    }
    selected_obj = this;
}
sdf::Object::~Object() {

}


/******************************** sdf_bridge.h **********************************/

void sdf::deinit() 
{
    std::fill_n(nr_each_prim_types, nr_prim_types, 0);
    nr_groups = 0;
    serialized_objs.clear();
    if(root) {
        delete root;
        root = nullptr;
    }

    materials.clear();
    for(Material* mat: materials) {
        delete mat;
    }

    selected_edit_mode_idx = 1;
    gzmo_space = ImGuizmo::MODE::LOCAL;
    
    ib_light.deinitGL();

    model_name = "Untitled";
}
static void resetData() {
    sdf::deinit();

    camera->pivot = glm::vec3(0,0,0);
    camera->position = glm::vec3(0,1,5);
    camera->updateViewMat();

    
    root = new sdf::Group("root", nullptr);

    addMaterial();
    selected_mat->name = "Material_floor";
    selected_mat->roughness = 1.0;
    selected_mat->metalness = 0.0;
    selected_mat->base_color = glm::vec3(1);
    selected_mat->updateShaderData();

    root->addObjectToBack(PT_BOX);
    root->children.back()->name = "Floor";
    root->children.back()->position = {0,-1,0};
    root->children.back()->scale = {20, 1, 20};
    root->children.back()->composeTransform();

    addMaterial();
    root->addObjectToBack(PT_BOX);
    sdf::serializeModel();
}
void sdf::init(CameraController* cam, Light* lit) {
    camera = cam;
    light = lit;

    resetData();
}

void sdf::bindSdfData(const Program& prog) 
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
    
    prog.setUniform("nr_objs", (int)serialized_objs.size());
    prog.setUniform("transforms", MAX_OBJS, transforms);
    prog.setUniform("scaling_factors", MAX_OBJS, scaling_factors);
    prog.setUniform("mat_idxs", MAX_OBJS, mat_idxs);
    prog.setUniform("prim_types", MAX_OBJS, prim_types);
    prog.setUniform("op_types", MAX_OBJS, op_types);
    prog.setUniform("blendnesses", MAX_OBJS, blendnesses);
    prog.setUniform("roundnesses", MAX_OBJS, roundnesses);

    prog.setUniform("donuts", MAX_PRIMS, donuts);
    prog.setUniform("capsules", MAX_PRIMS, capsules);

    prog.setUniform("use_IBL", use_IBL);
    if(use_IBL||true) {
        int activeSlot = 0;
        prog.setTexture("map_Light", ib_light.getTexIdLight(), activeSlot++);
        prog.setTexture("map_Irradiance", ib_light.getTexIdIrradiance(), activeSlot++);
        prog.setTexture3d("map_PreFilteredEnv", ib_light.getTexIdPreFilteredEnv(), activeSlot++);
        prog.setTexture("map_PreFilteredBRDF", ib_light.getTexIdPreFilteredBRDF(), activeSlot++);
    }
}


static sdf::Node* toMoveHirarchySrc = nullptr;
static sdf::Group* toMoveHirarchyDst = nullptr;
static sdf::Node* toDelHirarchySrc = nullptr;

// return is have to delete
// true : this obj have to delete
// falue : do not delete
static void drawHierarchyView(sdf::Node* nod) 
{
    bool isGroup = nod->is_group;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if( isGroup) {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_DefaultOpen;
    }
    else {
        flags |= ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
    }
    if(selected_obj == nod) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isOpen = ImGui::TreeNodeEx(nod, flags, "%s", nod->name.c_str());

    if( selected_obj!=nod &&(ImGui::IsItemClicked(0)||ImGui::IsItemClicked(1))) {
        selected_obj = nod;
        if(!isGroup) {
            selected_mat = ((sdf::Object*)nod)->p_mat;
        }
    }

    if( ImGui::BeginPopupContextItem() ) {
        if( isGroup && ImGui::BeginMenu("Add") ) {
            if( ImGui::MenuItem("Group", "Ctrl+G", false, true) ) {
                ((sdf::Group*)nod)->addGroupToBack();
            }
            for(int i=0; i<nr_prim_types; i++) {
                if( ImGui::MenuItem(prim_type_names[i], fmtStrToBuf("Ctrl+%d",i), false, true) ) {
                    ((sdf::Group*)nod)->addObjectToBack((PrimitiveType)i);
                    sdf::serializeModel();
                }
            }
            ImGui::EndMenu();
        }
        // root 삭제 불가능
        if( nod->parent && ImGui::MenuItem("Delete", "Backspace", false, true) ) {
            toDelHirarchySrc = nod;
            ImGui::TextUnformatted(nod->name.c_str());
            log::pure("delete\n");
        }
        ImGui::EndPopup();
    }

    // root 옮기기 불가능
    if( nod->parent && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None) ) {
        toMoveHirarchySrc = nod;
        ImGui::SetDragDropPayload("DND_SCENE_CELL", nullptr, 0);

        ImGui::EndDragDropSource();
    }
    if( isGroup && ImGui::BeginDragDropTarget() ) {
        if(ImGui::AcceptDragDropPayload("DND_SCENE_CELL")) {
            toMoveHirarchyDst = (sdf::Group*)nod;
            lim::log::pure("%s to %s\n",toMoveHirarchySrc->name.c_str(), toMoveHirarchyDst->name.c_str());
        }
        ImGui::EndDragDropTarget();
    }
    

    if( isGroup&&isOpen ) {
        sdf::Group* grp = (sdf::Group*)nod;
        for( sdf::Node* child: grp->children )
        {
            drawHierarchyView( child );
        }
        ImGui::TreePop();
    }
}

extern void exportMesh(std::string_view dir, std::string_view modelName, int sampleRate);

void sdf::drawImGui() 
{
    /* menu bar */
    if( ImGui::BeginMainMenuBar() )
    {
        static bool isJsonExporterOpened = false;
        static bool isMeshExporterOpened = false;
        static constexpr ImVec2 popupSize = {300, 300};
        static constexpr ImVec2 doneSize = {280, 100};

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
            static int sampleRate = 50;
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

        if( ImGui::MenuItem("Clear") ) {
            resetData();
            ImGui::EndMainMenuBar();
            return;
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

        drawHierarchyView(root);
        if(toDelHirarchySrc) {
            toDelHirarchySrc->parent->rmChild(toDelHirarchySrc);
            toDelHirarchySrc = nullptr;
        }
        if(toMoveHirarchyDst&&toMoveHirarchySrc) {
            toMoveHirarchyDst->getOtherChild(toMoveHirarchySrc);
            toMoveHirarchyDst = nullptr;
            toMoveHirarchySrc = nullptr;
            serializeModel();
        }

        // ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*2.3f);
        // ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+ImGui::GetContentRegionAvail().y-ImGui::GetFontSize()*1.6f);
        if( ImGui::Button("Add", {-1,0}) ) {
            ImGui::OpenPopup("AddNodePopup");
        }
        if( ImGui::BeginPopup("AddNodePopup") ) {
            if( !selected_obj->is_group ) {
                selected_obj = selected_obj->parent;
            }
            sdf::Group* grp = (sdf::Group*)selected_obj;
            if( ImGui::Selectable("Group") ) {
                grp->addGroupToBack();
            }
            for(int i=0; i<nr_prim_types; i++) {
                if( ImGui::Selectable(prim_type_names[i]) ) {
                    grp->addObjectToBack((PrimitiveType)i);
                    serializeModel();
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
        sdf::Node* nod = selected_obj;
        ImGui::InputText("Name", &nod->name, ImGuiInputTextFlags_AutoSelectAll);

        static const float slide_pos_spd = 4/500.f;
        static const float slide_scale_spd = 4/500.f;
        static const float slide_rot_spd = 180/500.f;
        if(ImGui::DragFloat3("Position", glm::value_ptr(nod->position), slide_pos_spd, -FLT_MAX, +FLT_MAX)) {
            nod->composeTransform();
        }
        if(ImGui::DragFloat3("Scale", glm::value_ptr(nod->scale), slide_scale_spd, -FLT_MAX, +FLT_MAX)) {
            nod->composeTransform();
        }
        if(ImGui::DragFloat3("Rotate", glm::value_ptr(nod->euler_angles), slide_rot_spd, -FLT_MAX, +FLT_MAX)) {
            nod->composeTransform();
        }
        if( LimGui::CheckBox3("Mirror", glm::value_ptr(nod->mirror)) ) {

        }
        if( !nod->is_group ) {
            sdf::Object& obj = *((sdf::Object*)selected_obj);
            if( ImGui::Combo("OperGroup", (int*)&nod->op_group, prim_oper_group_names, nr_prim_oper_groups) ) {
                op_types[obj.idx] = nod->op_group*2 + nod->op_spec;
            }
            if( ImGui::Combo("OperSpec", (int*)&nod->op_spec, prim_oper_spec_names, nr_prim_oper_specs) ) {
                op_types[obj.idx] = nod->op_group*2 + nod->op_spec;
            }
            if( ImGui::SliderFloat("Blendness", &nod->blendness, 0.f, 1.f) ) {
                blendnesses[obj.idx] = nod->blendness;
            }
            // if( ImGui::SliderFloat("Roundness", &nod->roundness, 0.f, 1.f) ) {
            //     roundnesses[obj.idx] = nod->roundness;
            // }
            int matIdx = obj.p_mat->idx;
            if( ImGui::Combo("Material", &matIdx, mat_names, materials.size()+1) ) {
                if( matIdx<materials.size() ) {
                    selected_mat = materials[matIdx];
                } else {
                    addMaterial();
                }
                obj.p_mat = selected_mat;
                mat_idxs[obj.idx] = matIdx;
            } 
            ImGui::Separator();
            switch(obj.prim_type) {
            case PT_DONUT:
                ImGui::DragFloat("radius", &donuts[obj.prim_idx], 0.02f, 0.1f, 5.f);
                break;
            }
        }
        
        ImGui::End();
    }
    /* Settings */
    {
        ImGui::Begin("Settings##sdf");
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        if(ImGui::Checkbox("use IBL", &use_IBL)&& !ib_light.is_map_baked) {
            ib_light.setMap("assets/ibls/Alexs_Apt_2k.hdr");
            ib_light.bakeMap();
        }
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
    /* Mat List */
    {
        ImGui::Begin("Materials##sdf");
        
        ImGuiTreeNodeFlags tnFlags = ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_Bullet;
        for( sdf::Material* mat: materials ) {
            ImGuiTreeNodeFlags myTnFlags = tnFlags;
            if(mat == selected_mat) {
                myTnFlags |= ImGuiTreeNodeFlags_Selected;
            }
            if( ImGui::TreeNodeEx("MatList", myTnFlags, "%s", mat->name.c_str()) ) {
                if( mat != selected_mat &&(ImGui::IsItemClicked(0)||ImGui::IsItemClicked(1))) {
                    selected_mat = mat;
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
        sdf::Material& mat = *selected_mat;
        ImGui::InputText("Name", &mat.name, ImGuiInputTextFlags_AutoSelectAll);
        if( ImGui::SliderFloat("Roughness", &mat.roughness, 0.001, 1.0) ) {
            roughnesses[mat.idx] = mat.roughness;
        }
        if( ImGui::SliderFloat("Metalness", &mat.metalness, 0.001, 1.0) ) {
            metalnesses[mat.idx] = mat.metalness;
        }
        ImGui::TextUnformatted("Base Color");
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        float w = glm::min(280.f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.y);
        ImGui::SetNextItemWidth(w);
        if( ImGui::ColorPicker3("##Base Color", glm::value_ptr(mat.base_color), flags) ) {
            base_colors[mat.idx] = glm::convertSRGBToLinear(mat.base_color);
        }
        ImGui::End();
    }
}


void sdf::drawGuizmo(const Viewport& vp) {
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
    // ImGuizmo::ViewManipulate( glm::value_ptr(cam.view_mat), 8.0f, ImVec2{pos.x+size.x-128, pos.y}, ImVec2{128, 128}, (ImU32)0x10101010 );


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
}


// Todo : picking with framebuffer
void sdf::clickCallback(int btn, glm::vec2 uv) {
    if(selected_edit_mode_idx!=0)
        return;

    float eyeZ = 1.f/tan(glm::radians(camera->fovy)/2.f);
    glm::vec3 pickDir, front, right, up;
    front = glm::normalize(camera->pivot - camera->position);
    right = glm::normalize( glm::cross(front, {0,1,0}) );
    up = glm::normalize( glm::cross(right, front) );
    uv = uv*2.f-glm::vec2(1);
    uv *= glm::vec2(camera->aspect,-1);
    pickDir = glm::normalize( uv.x*right + uv.y*up + eyeZ*front );
    

    float minDist = 999999.f;
    for(Object* obj: serialized_objs) {
        glm::vec3 toObj = glm::vec3(obj->transform[3])-camera->position;
        float rr = obj->getScaleFactor(); rr *= rr;
        float distFromLine2 = glm::length2( glm::cross(pickDir, toObj) );
        float distProjLine = glm::dot(pickDir, toObj);

        if( distFromLine2 < rr ) {
            if( distProjLine>0 && distProjLine<minDist ) {
                selected_obj = obj;
                selected_edit_mode_idx = 1;
            }
        }
    }
}

static sdf::Node* copied_sdf_node = nullptr;

void sdf::keyCallback(int key, int scancode, int action, int mods) {
    if( action!=GLFW_PRESS )
        return;
    if( mods==0 ) {
        switch(key) {
        case GLFW_KEY_Z: selected_edit_mode_idx = 0; break;
        case GLFW_KEY_X: selected_edit_mode_idx = 1; break;
        case GLFW_KEY_C: selected_edit_mode_idx = 2; break;
        case GLFW_KEY_V: selected_edit_mode_idx = 3; break;
        case GLFW_KEY_B: selected_edit_mode_idx = 4; break;
        }
    }
    else if( mods==GLFW_MOD_CONTROL ) {
        switch(key) {
        case GLFW_KEY_C:
            copied_sdf_node = selected_obj;
            break;
        case GLFW_KEY_V:
            Group* dstGroup;
            if(selected_obj->is_group)
                dstGroup = (Group*)selected_obj;
            else 
                dstGroup = selected_obj->parent;
            copied_sdf_node->copyToGroup(dstGroup);
            serializeModel();
            break;
        }
    }
}