/*
    정의는 sdf_lib.c에 있다.
*/

#ifndef __sdf_global_h_
#define __sdf_global_h_

#include "sdf_obj.h"
#include <imgui.h>
#include <imguizmo/ImGuizmo.h>
#include <limbrary/3d/light.h>
#include <limbrary/3d/scene.h>
#include <limbrary/3d/viewport_with_cam.h>

/************* glsl data **************/
constexpr int MAX_MATS = 32;
constexpr int MAX_OBJS = 32;
constexpr int MAX_PRIMS = 32;

// optimize options
extern int nr_march_steps;
extern float far_distance;
extern float hit_threshold;
extern float diff_for_normal;

// mat
extern glm::vec3 base_colors[MAX_MATS]; // linear space
extern float roughnesses[MAX_MATS];
extern float metalnesses[MAX_MATS];
extern glm::vec3 sky_color;

// obj
// nr_objs = serialized_objs.size()
extern glm::mat4 transforms[MAX_OBJS]; // inversed
extern float scaling_factors[MAX_OBJS];
extern int mat_idxs[MAX_OBJS];
extern int op_types[MAX_OBJS];   // Todo: runtime edit shader
extern int prim_types[MAX_OBJS]; // Todo: runtime edit shader
extern int prim_idxs[MAX_OBJS];
extern float blendnesses[MAX_OBJS];
extern float roundnesses[MAX_OBJS];

// each prim ...
extern float donuts[MAX_PRIMS];
extern float capsules[MAX_PRIMS];

extern lim::IBLight* ib_light;
extern bool use_IBL;


/***************** application data ****************/
extern int nr_each_prim_types[nr_prim_types];
extern int nr_groups;
extern std::vector<sdf::Object*> serialized_objs;
extern sdf::Node* selected_obj;
extern sdf::Material* selected_mat;

extern sdf::Group* root;

extern std::vector<sdf::Material*> materials;
extern const char* mat_names[MAX_MATS];

constexpr ImGuizmo::OPERATION gzmo_edit_modes[] = { 
    (ImGuizmo::OPERATION)0, ImGuizmo::TRANSLATE, 
    ImGuizmo::SCALE, ImGuizmo::ROTATE, ImGuizmo::UNIVERSAL 
};
extern int selected_edit_mode_idx;
extern ImGuizmo::MODE gzmo_space;

extern std::string model_name;
extern lim::CameraCtrl* camera;
extern lim::LightDirectional* light;


namespace sdf {
    void serializeModel();
}


#endif