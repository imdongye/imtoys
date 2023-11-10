/*

2023-11-02 / im dong ye

(1 - 1 + 1) 

*/

#ifndef __sdf_h_
#define __sdf_h_

#include <vector>
#include <string>
#include <limbrary/model_view/transform.h>

namespace lim { namespace sdf 
{
    enum PrimitiveType {
        PT_GROUP,
        PT_SPHERE,
        PT_BOX,
        PT_PIPE,
        PT_DONUT,
    };
    struct Donut {
        float radius;
    };

    struct Material {
        glm::vec3 base_color;
        float roughness = 1.f;
        float metalness = 1.f;
    };

    enum OperationType {
        OT_ADD, OT_SUB
    };
    struct Object : public TransformWithInv {
        std::string name {"sdf_obj"};
        int mat_idx {0};
        PrimitiveType prim_type {PT_GROUP};
        OperationType op_type {OT_ADD};
        float blendness = 0.f;
        float roundness = 0.f;
        glm::bvec3 mirror {0,0,0};
    };
    struct ObjectNode : public Object {
        ObjectNode* parent;
        std::vector<ObjectNode*> children;
    };
    struct Data {
        std::vector<Material> mats;
        Object root;
        std::vector<Object> rst_obj;
    };
}}

#endif