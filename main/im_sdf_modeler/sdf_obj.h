#ifndef __sdf_obj_h_
#define __sdf_obj_h_

#include <string>
#include <glm/glm.hpp>
#include <vector>

enum PrimitiveType {
    PT_SPHERE, PT_BOX, PT_PIPE, PT_DONUT
};
constexpr int nr_prim_types = 4;
inline const char* prim_type_names[nr_prim_types] = {
    "Sphere", "Box", "Pipe", "Donut"
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

namespace sdf 
{
    struct Material {
        std::string name = "sdf_mat";
        int idx;
        glm::vec3 base_color = {0.2, 0.13, 0.87};
        float roughness = 1.f;
        float metalness = 0.f;
        void updateShaderData();
    };
    
    struct Group;
    struct Node {
        std::string name = "sdf_obj";
        bool is_group = false;

        Group* parent = nullptr;
        glm::mat4 transform = glm::mat4(1); // global transform
        glm::vec3 position = {0,0,0};
        glm::vec3 scale = glm::vec3(1);
        glm::vec3 euler_angles = glm::vec3(0);
        glm::mat4 my_transform = glm::mat4(1); // local transform
        glm::bvec3 mirror = {0,0,0};
        int op_group = OG_ADDITION; // addition
        int op_spec = OS_ROUND;  // round
        float blendness = 0.f;
        float roundness = 0.f;

        Node() = default;
        Node(std::string_view name, Group* parent);
        virtual ~Node() = default;
        void updateTransformWithParent();
        void composeTransform();
        void decomposeTransform();
        float getScaleFactor();
        void copyDataTo(Node* dst);
        // you must call serializeModel() after call this func
        virtual void copyToGroup(Group* group) = 0;
    };

    struct Group: public Node {
        std::vector<Node*> children;
        Group(std::string_view name, Group* parent);
        ~Group();
        void addGroupToBack();
        void addObjectToBack(PrimitiveType pt);
        void rmChild(sdf::Node* child);
        // to move
        void getOtherChild(sdf::Node* child);
        virtual void copyToGroup(Group* group) override;
    };

    struct Object: public Node {
        int idx = 0;
        int prim_type = PT_BOX;
        Material* p_mat = nullptr;
        int prim_idx = 0;

        Object(std::string_view name, Group* parent, PrimitiveType pt);
        ~Object();
        void updateShaderData();
        virtual void copyToGroup(Group* group) override;
    };
}

#endif