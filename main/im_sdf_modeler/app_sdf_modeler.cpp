/*
 	Note:

	Todo:
	glsl uniform 배열들 texture에 저장해서 넘기기
*/

#include "app_sdf_modeler.h"
#include <imgui.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/asset_lib.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lim { namespace sdf
{
	constexpr int nr_oper_types = 5;
	const char* prim_oper_names[nr_oper_types] = {
        "Addition", "Subtitution"
    };
    enum OperationType {
        OT_ADDITION, OT_SUBTRACTION, OT_INTERSECTION
    };
	constexpr int nr_prim_types = 5;
	const char* prim_type_names[nr_prim_types] = {
        "Group", "Sphere", "Box", "Pipe", "Donut"
    };
    enum PrimitiveType {
        PT_GROUP,
        PT_SPHERE,
        PT_BOX,
        PT_PIPE,
        PT_DONUT,
    };
    struct Material {
        glm::vec3 base_color;
        float roughness = 1.f;
        float metalness = 1.f;
    };
    struct ObjNode {
        int obj_idx = -1;
        glm::mat4 translate_mat = glm::mat4(1);
        glm::mat4 scale_mat = glm::mat4(1);
        glm::mat4 rotate_mat = glm::mat4(1);

		// 8개의 속성은 group이 아니라면 부모에 따라 수정되어 glsl data에 복사됨.
        glm::mat4 transform = glm::mat4(1);
        int mat_idx = 0;
        PrimitiveType prim_type = PT_GROUP;
        int prim_idx = 0;
        OperationType op_type = OT_ADDITION;
        float blendness = 0.f;
        float roundness = 0.f;
        glm::bvec3 mirror = {0,0,0};

        std::string name = "sdf_obj";
        glm::vec3 position = {0,0,0};
        glm::vec3 scale = glm::vec3(1);
        glm::vec3 euler_angles = glm::vec3(0);
        const ObjNode* parent = nullptr;
        std::vector<ObjNode> children;

		ObjNode() = default;
		ObjNode(PrimitiveType primType, const ObjNode* parent);
        void addChild(PrimitiveType primType);
		void updateTranslate();
		void updateScale();
		void updateRotate();
	private:
		void makeTransform();
    };

	 /* application data */
	namespace {
		ObjNode root;
		ObjNode* selected_obj = nullptr;
		std::vector<Material> mats;
		int nr_prims[nr_prim_types] = {0,};
	};

	/* glsl data */
	// todo: 1.uniform block   2.look up table texture
	namespace {
		constexpr int MAX_MATS = 32;
		constexpr int MAX_OBJS = 32;
		constexpr int MAX_PRIMS = 32;

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
		glm::ivec3 mirrors[2*MAX_OBJS];

		// each prim ...
		float donuts[MAX_PRIMS];
		float cylinder[MAX_PRIMS];
	}

	ObjNode::ObjNode(PrimitiveType primType, const ObjNode* p)
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
		mirrors[obj_idx] = mirror;
		makeTransform();
	}
	void ObjNode::addChild(PrimitiveType primType)
	{
		children.emplace_back(primType, this);
	}
	void ObjNode::updateTranslate() {
		translate_mat = glm::translate(position);
		makeTransform();
	}
	void ObjNode::updateScale() {
		scale_mat = glm::scale(scale);
		makeTransform();
	}
	void ObjNode::updateRotate() {
		rotate_mat = glm::eulerAngleYXZ(euler_angles.y, euler_angles.x, euler_angles.z);
		makeTransform();
	}
	void ObjNode::makeTransform() {
		transform = translate_mat * rotate_mat * scale_mat;
		if(obj_idx<0)
			return;
		transforms[obj_idx] = glm::inverse(transform);
	}

	void makeSpaceInGlslObjData(int pos, int len) {
		int nr_rights = nr_objs - pos;
		memcpy((float*)&transforms[pos+len], (float*)&transforms[pos], sizeof(glm::mat4)*nr_rights);
		memcpy((int*)&mat_idxs[pos+len], (int*)&mat_idxs[pos], sizeof(int)*nr_rights);
		memcpy((int*)&prim_types[pos+len], (int*)&prim_types[pos], sizeof(int)*nr_rights);
		memcpy((int*)&prim_idxs[pos+len], (int*)&prim_idxs[pos], sizeof(int)*nr_rights);
		memcpy((int*)&op_types[pos+len], (int*)&op_types[pos], sizeof(int)*nr_rights);
		memcpy((float*)&blendnesses[pos+len], (float*)&blendnesses[pos], sizeof(float)*nr_rights);
		memcpy((float*)&roundnesses[pos+len], (float*)&roundnesses[pos], sizeof(float)*nr_rights);
		memcpy((int*)&mirrors[pos+len], (int*)&mirrors[pos], sizeof(glm::ivec3)*nr_rights);
	}

	void init() 
	{
        mats.push_back({{0.2, 0.13, 0.87}, 1.f, 0.f});
        root.name = "root";
        
        root.addChild(PT_BOX);
        selected_obj = &root.children.back();
    }
    void deinit() 
	{
        nr_objs = 0;
        selected_obj = nullptr;
		for(int i=0;i<nr_prim_types;i++) {
			nr_prims[i] = 0;
		}
        mats.clear();
    }
    void bindSdfData(const Program& prog) 
	{
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
        prog.setUniform("mirrors", MAX_OBJS, mirrors);
    }

    void drawTree(ObjNode* pObj) 
	{
		ObjNode& obj = *pObj;
        bool isGroup = obj.prim_type==PT_GROUP;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnArrow;
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
			if( obj.prim_type==PT_GROUP && ImGui::BeginMenu("Add") ) {
				for(int i=0; i<nr_prim_types; i++) {
					if( ImGui::MenuItem(prim_type_names[i], fmtStrToBuf("Ctrl+%d",i), false, true) ) {
						obj.addChild((PrimitiveType)i);
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
                drawTree(&child);
            }
            ImGui::TreePop();
        }
    }

    void drawImGui() 
	{
        /* Scene Hierarchy */
        {
            ImGui::Begin("Scene##sdf");
            drawTree(&root);
            ImGui::End();
        }
        /* Obj Editor */
        {
            ImGui::Begin("Object##sdf");
            if(!selected_obj) {
                ImGui::End();
                return;
            }
            ObjNode& obj = *selected_obj;
            ImGui::Text("%s", obj.name.c_str());

            static const float slide_spd = 1/300.f;
            static bool isMatUpdated = false;
            if(ImGui::DragFloat3("pos", glm::value_ptr(obj.position), slide_spd, -FLT_MAX, +FLT_MAX)) {
                obj.updateTranslate();
            }
            if(ImGui::DragFloat3("scale", glm::value_ptr(obj.scale), slide_spd, -FLT_MAX, +FLT_MAX)) {
                obj.updateScale();
            }
            if(ImGui::DragFloat3("rotate", glm::value_ptr(obj.euler_angles), slide_spd, -FLT_MAX, +FLT_MAX)) {
                obj.updateRotate();
            }
            
            ImGui::End();
        }
        /* Mat List*/
        {
            ImGui::Begin("Materials##sdf");
            ImGui::End();
        }
        /* Mat Editor */
        {
            ImGui::Begin("Mat Editor##sdf");
            ImGui::End();
        }
    }
}}



namespace lim
{
	AppSdfModeler::AppSdfModeler(): AppBase(1373, 783, APP_NAME, false)
		, viewport("AnimTester", new Framebuffer()) // 멀티셈플링 동작 안함.
	{
		GLint tempInt;
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &tempInt);
		log::pure("GL_MAX_FRAGMENT_UNIFORM_VECTORS : %d\n", tempInt);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &tempInt);
		log::pure("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS : %d\n", tempInt);

		viewport.use_guizmo = true;
		viewport.camera.pivot = glm::vec3(0,1,0);
		viewport.camera.position = glm::vec3(0,1,5);
		viewport.camera.updateViewMat();
		prog.name = "sdf and ray marching";
		prog.home_dir = APP_DIR;
		prog.attatch("canvas.vs").attatch("shader.fs").link();

	

		sdf::init();
	}
	AppSdfModeler::~AppSdfModeler()
	{
		sdf::deinit();
	}
	void AppSdfModeler::update()
	{
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Camera& cam = viewport.camera;
		viewport.getFb().bind();
		prog.use();
		prog.setUniform("lightPos", light.position);
		prog.setUniform("lightInt", light.intensity);
		prog.setUniform("cameraAspect", cam.aspect);
		prog.setUniform("cameraFovy", cam.fovy);
		prog.setUniform("cameraOrthWidth", 0.f);
		prog.setUniform("cameraPos", cam.position);
		prog.setUniform("cameraPivot", cam.pivot);
		sdf::bindSdfData(prog);

		AssetLib::get().screen_quad.drawGL();
		viewport.getFb().unbind();
	}
	void AppSdfModeler::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport.drawImGui();

		sdf::drawImGui();
	}
	void AppSdfModeler::keyCallback(int key, int scancode, int action, int mods) {
		if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
			prog.reload(GL_FRAGMENT_SHADER);
		}
	}
}
