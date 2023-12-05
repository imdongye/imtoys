#ifndef __sdf_obj_h_
#define __sdf_obj_h_

#include <string>
#include <glm/glm.hpp>

enum PrimitiveType {
    PT_GROUP, PT_SPHERE, PT_BOX, PT_PIPE, PT_DONUT
};
constexpr int nr_prim_types = 5;
inline const char* prim_type_names[nr_prim_types] = {
    "Group", "Sphere", "Box", "Pipe", "Donut"
};

enum OperationGroup {
    OG_ADDITION,
    OG_SUBTITUTION,
    OG_INTERSECTION,
};
constexpr int nr_prim_oper_groups = 3;
inline const char* prim_oper_group_names[nr_prim_oper_groups] = {
    "Addition", "Subtitution", "Intersection"
};
enum OperationSpec {
    OS_ROUND,
    OS_EDGE,
};
constexpr int nr_prim_oper_specs = 2;
inline const char* prim_oper_spec_names[nr_prim_oper_specs] = {
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

struct ObjNode {
    int obj_idx = -1;

    // 7개의 속성은 group이 아니라면 부모에 따라 수정되어 glsl data에 복사됨.
    int mat_idx = 0;
    int prim_type = PT_GROUP;
    int prim_idx = 0;
    int op_group = OG_ADDITION; // addition
    int op_spec = OS_ROUND;  // round
    float blendness = 0.f;
    float roundness = 0.f;
    // RecomposeMatrixFromComponents로 생성됨.
    glm::mat4 transform = glm::mat4(1); // global transform

    std::string name = "sdf_obj";
    glm::vec3 position = {0,0,0};
    glm::vec3 scale = glm::vec3(1);
    glm::vec3 euler_angles = glm::vec3(0);
    glm::mat4 my_transform = glm::mat4(1); // local transform
    glm::bvec3 mirror = {0,0,0};
    ObjNode* parent = nullptr;
    std::vector<ObjNode*> children;

    ObjNode() = default;
    ObjNode(std::string_view _name, PrimitiveType primType, ObjNode* parent);
    ~ObjNode();

    void updateGlsl(); // without transform
    float getScaleFactor();
    void updateTransformWithParent();
    void composeTransform();
    void decomposeTransform();
};

struct SdfMaterial {
    std::string name = "sdf_mat";
    int idx = 0;
    glm::vec3 base_color = {0.2, 0.13, 0.87};
    float roughness = 1.f;
    float metalness = 0.f;
    
    SdfMaterial() = default;
    SdfMaterial(std::string_view _name);
    void updateGlsl();
};

#endif