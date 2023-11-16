#include <limbrary/model_view/camera_man.h>
#include <limbrary/program.h>

namespace lim { namespace sdf
{
    struct ObjNode {
         enum OperationType {
            OT_ADDITION, OT_SUBTRACTION, OT_INTERSECTION
        };
        enum PrimitiveType {
            PT_GROUP, PT_SPHERE, PT_BOX, PT_PIPE, PT_DONUT
        };
        int obj_idx = -1;

		// 7개의 속성은 group이 아니라면 부모에 따라 수정되어 glsl data에 복사됨.
        glm::mat4 transform = glm::mat4(1); // global transform
        int mat_idx = 0;
        PrimitiveType prim_type = PT_GROUP;
        int prim_idx = 0;
        OperationType op_type = OT_ADDITION;
        float blendness = 0.f;
        float roundness = 0.f;

        std::string name = "sdf_obj";
        glm::vec3 position = {0,0,0};
        glm::vec3 scale = glm::vec3(1);
        glm::vec3 euler_angles = glm::vec3(0);
        glm::mat4 my_transform = glm::mat4(1); // local transform
        ObjNode* parent = nullptr;
        std::vector<ObjNode> children;
        glm::bvec3 mirror = {0,0,0};

		ObjNode() = default;
		ObjNode(PrimitiveType primType, ObjNode* parent);
        void addChild(PrimitiveType primType);
		void updateTransformWithParent();
		void composeTransform();
		void decomposeTransform();
    };

    struct Material {
		std::string name = "sdf_mat";
        glm::vec3 base_color;
        float roughness = 1.f;
        float metalness = 1.f;
    };
	void makeSpaceInGlslObjData(int pos, int len);
	void init();
    void deinit();
    void bindSdfData(const Program& prog);
    void drawObjTree(ObjNode* pObj);
    void drawImGui();
	void drawGuizmo(const Viewport& vp, Camera& cam);
}}