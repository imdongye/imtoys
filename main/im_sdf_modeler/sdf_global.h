#include <glm/glm.hpp>

namespace 
{
    enum PrimitiveType {
        PT_GROUP, PT_SPHERE, PT_BOX, PT_PIPE, PT_DONUT
    };
    constexpr int nr_prim_types = 5;
    const char* prim_type_names[nr_prim_types] = {
        "Group", "Sphere", "Box", "Pipe", "Donut"
    };

    enum OperationGroup {
        OG_ADDITION,
        OG_SUBTITUTION,
        OG_INTERSECTION,
    };
    constexpr int nr_prim_oper_groups = 3;
    const char* prim_oper_group_names[nr_prim_oper_groups] = {
        "Addition", "Subtitution", "Intersection"
    };
    enum OperationSpec {
        OS_ROUND,
        OS_EDGE,
    };
    constexpr int nr_prim_oper_specs = 2;
    const char* prim_oper_spec_names[nr_prim_oper_specs] = {
        "Round", "Edge",
    };

    enum OperationType {
        OT_ADD_ROUND, // addition
        OT_ADD_EDGE,
        OT_SUB_ROUND, // subtitution
        OT_SUB_EDGE,
        OT_INT_ROUND, // intersection
        OT_INT_EDGE,
    };
}

/************* glsl data **************/
// todo: 1.uniform block   2.look up table texture

constexpr int MAX_MATS = 32;
constexpr int MAX_OBJS = 32;
constexpr int MAX_PRIMS = 32;

// optimize options
extern int nr_march_steps;
extern float far_distance;
extern float hit_threshold;
extern float diff_for_normal;

// env
extern glm::vec3 sky_color;

// mat
extern glm::vec3 base_colors[MAX_MATS]; // linear space
extern float roughnesses[MAX_MATS];
extern float metalnesses[MAX_MATS];

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
extern float donuts[MAX_PRIMS];
extern float cylinder[MAX_PRIMS];

/*******************************************/