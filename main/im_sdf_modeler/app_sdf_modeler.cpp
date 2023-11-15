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
#include <imguizmo/ImGuizmo.h>
#include <limbrary/limgui.h>


namespace lim { namespace sdf
{
	namespace {
		constexpr int nr_oper_types = 3;
		const char* prim_oper_names[nr_oper_types] = {
			"Addition", "Subtitution", "Intersection"
		};
		enum OperationType {
			OT_ADDITION, OT_SUBTRACTION, OT_INTERSECTION
		};

		constexpr int nr_prim_types = 5;
		const char* prim_type_names[nr_prim_types] = {
			"Group", "Sphere", "Box", "Pipe", "Donut"
		};
		enum PrimitiveType {
			PT_GROUP, PT_SPHERE, PT_BOX, PT_PIPE, PT_DONUT
		};
	}

    struct Material {
        glm::vec3 base_color;
        float roughness = 1.f;
        float metalness = 1.f;
    };
    struct ObjNode {
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
        const ObjNode* parent = nullptr;
        std::vector<ObjNode> children;
        glm::bvec3 mirror = {0,0,0};

		ObjNode() = default;
		ObjNode(PrimitiveType primType, const ObjNode* parent);
        void addChild(PrimitiveType primType);
		void updateTransformWithParent();
		void composeTransform();
		void decomposeTransform();
    };

	/* application data */
	namespace {
		ObjNode root;
		ObjNode* selected_obj = nullptr;
		std::vector<Material> mats;
		int nr_prims[nr_prim_types] = {0,};

		ImGuizmo::OPERATION gzmo_edit_mode = ImGuizmo::OPERATION::TRANSLATE;
		ImGuizmo::MODE 		gzmo_space = ImGuizmo::MODE::LOCAL;
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

		// each prim ...
		float donuts[MAX_PRIMS];
		float cylinder[MAX_PRIMS];
	}

	/* functions */

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

		composeTransform();
	}
	void ObjNode::addChild(PrimitiveType primType)
	{
		children.emplace_back(primType, this);
	}
	void ObjNode::updateTransformWithParent() {
		if(parent) {
			transform = parent->transform * my_transform;
		}
		if(obj_idx<0) {
			for(ObjNode& child : children) {
				child.updateTransformWithParent();
			}
		}
		else {
			transforms[obj_idx] = glm::inverse(transform);
		}
	}
	void ObjNode::composeTransform() {
		ImGuizmo::RecomposeMatrixFromComponents(
					glm::value_ptr(position)
					,glm::value_ptr(euler_angles)
					,glm::value_ptr(scale)
					,glm::value_ptr(transform));
		
		my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;
		updateTransformWithParent();
	}
	void ObjNode::decomposeTransform() {
		my_transform = (parent)? glm::inverse(parent->transform)*transform : transform;

		ImGuizmo::DecomposeMatrixToComponents(
					glm::value_ptr(transform)
					,glm::value_ptr(position)
					,glm::value_ptr(euler_angles)
					,glm::value_ptr(scale));

		updateTransformWithParent();
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
			float windowWidth = ImGui::CalcItemWidth();
            if(!selected_obj) {
                ImGui::End();
                return;
            }
			ObjNode& obj = *selected_obj;
            ImGui::Text("%s", obj.name.c_str());

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
				// for(int i=0;i<3;i++) {
				// 	ImGui::PushID(i);
				// 	ImGui::Checkbox("",&obj.mirror[i]);
				// 	ImGui::PopID();
				// 	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
				// }
				// ImGui::Text("Mirror");
				LimGui::CheckBox3("Mirror", glm::value_ptr(obj.mirror));
			}
			else {
				if( ImGui::Combo("Operator", (int*)&obj.op_type, prim_oper_names, nr_oper_types) ) {
					//gzmo_oper = (ImGuizmo::OPERATION)gzmoOpers[selectecGzmoOperIdx];
				}
				if( ImGui::SliderFloat("Blendness", &obj.blendness, 0.f, 1.f) ) {
				}
				if( ImGui::SliderFloat("Roundness", &obj.roundness, 0.f, 1.f) ) {
				}
			}

            
            ImGui::End();
        }
		/* Settings */
		{
			static int nr_step = 100;
			ImGui::Begin("Settings##sdf");
				ImGui::Text("%f", ImGui::CalcItemWidth());

			ImGui::SliderInt("nr_iter", &nr_step, 20, 900, "%d");
				ImGui::Text("%f", ImGui::CalcItemWidth());


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
	void drawGuizmo(const Viewport& vp, Camera& cam) {
		const auto& pos = ImGui::GetItemRectMin();
		const auto& size = ImGui::GetItemRectSize();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

		ImGuizmo::SetOrthographic(false);
		if(gzmo_edit_mode>0) {
			ImGuizmo::Manipulate( glm::value_ptr(cam.view_mat), glm::value_ptr(cam.proj_mat)
								, gzmo_edit_mode, gzmo_space, glm::value_ptr(selected_obj->transform)
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
			ImVec2 windowPos = {workPos.x+PAD, workPos.y+PAD+PAD*1.8f};
			
			//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
			//ImGui::SetNextWindowSizeConstraints(ImVec2(30.f, 120.f));
			ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID); // 뷰포트가 안되도록
			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
			if( ImGui::Begin("edit mode selector", nullptr, window_flags) )
			{
				static int selectedEditModeIdx = 1;
				static const char* editModeStrs[] = { "Select", "Position", "Scale", "Rotate", "Transform" };
				static const int   editModes[] = { 0, ImGuizmo::TRANSLATE, ImGuizmo::SCALE, ImGuizmo::ROTATE, ImGuizmo::UNIVERSAL };
				for(int i=0; i<5; i++) {
					if( ImGui::Selectable(editModeStrs[i], selectedEditModeIdx==i, 0, {30, 30}) ) {
						selectedEditModeIdx = i;
						gzmo_edit_mode = (ImGuizmo::OPERATION)editModes[i];
					}
				}
			}
			//ImGui::PopStyleVar();
			ImGui::End();
		}
	}
}}



namespace lim
{
	AppSdfModeler::AppSdfModeler(): AppBase(1373, 783, APP_NAME, false)
		, viewport("AnimTester", new FramebufferNoDepth()) // 멀티셈플링 동작 안함.
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

		viewport.drawImGui([&cam = viewport.camera](const Viewport& vp){
			sdf::drawGuizmo(vp, cam);
		});

		sdf::drawImGui();
	}
	void AppSdfModeler::keyCallback(int key, int scancode, int action, int mods) {
		if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
			prog.reload(GL_FRAGMENT_SHADER);
		}
	}
}
