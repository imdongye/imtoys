/*
    정의는 sdf_lib.c에 있다.
*/

#ifndef __sdf_global_h_
#define __sdf_global_h_

#include "sdf_obj.h"
#include <imgui.h>
#include <imguizmo/ImGuizmo.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera_man.h>

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
extern int nr_objs;
extern glm::mat4 transforms[MAX_OBJS]; // inversed
extern float scaling_factors[MAX_OBJS];
extern int mat_idxs[MAX_OBJS];
extern int op_types[MAX_OBJS];   // Todo: runtime edit shader
extern int prim_types[MAX_OBJS]; // Todo: runtime edit shader
extern int prim_idxs[MAX_OBJS];
extern float blendnesses[MAX_OBJS];
extern float roundnesses[MAX_OBJS];

// each prim ...
extern glm::vec2 donuts[MAX_PRIMS];
extern float capsules[MAX_PRIMS];


/***************** application data ****************/
extern int nr_mats;

extern ObjNode root;
extern ObjNode* selected_obj;
extern int nr_prims[nr_prim_types];

extern SdfMaterial materials[MAX_MATS];
extern const char* mat_names[MAX_MATS];
extern int selected_mat_idx;

constexpr ImGuizmo::OPERATION gzmo_edit_modes[] = { 
    (ImGuizmo::OPERATION)0, ImGuizmo::TRANSLATE, 
    ImGuizmo::SCALE, ImGuizmo::ROTATE, ImGuizmo::UNIVERSAL 
};
extern int selected_edit_mode_idx;
extern ImGuizmo::MODE gzmo_space;

extern std::string model_name;
extern lim::CameraController* camera;
extern lim::Light* light;


#endif