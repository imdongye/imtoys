/*

2023-09-11 / im dong ye
edit learnopengl code

Note:
Model은 실제 Mesh데이터, Material(Texture) 데이터와 root node를 소유하는 객체이다.
Node는 Model의 데이터(또는 공용 asset)를 참조하는 객체이고 트리구조를 만들고 mesh가 사용하는 material을 명시한다.
Material은 Model의 texture데이터를 참조한다.
Model을 normalize하면 root 노드의 Transform에 크기와 위치를 노멀라이즈하는 변환행렬이 들어가고 메쉬와 변환은 그 다음 노드부터 시작된다.

TODO list:
1. Transfrom.h
2. rigging
3. not gl_static으로 실시간 vert변화
4. width, height, depth 찾아서 -1~1공간으로 scaling
5. load model 이 모델안에 있는데 따로 빼야될까
6. 언제 어디서 업데이트해줘야하는지 규칙정하기
7. reload normal map 외부로 빼기
8. export animations

Model에 의존하지 않고 RdNode에 의존하는 Scene

*/

#ifndef __model_h_
#define __model_h_

#include <string>
#include <vector>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include "../texture.h"
#include "mesh.h"
#include "camera.h"
#include "transform.h"
#include "animator.h"
#include "material.h"
#include "mesh_skinned.h"

namespace lim
{
	struct RdNode {
		struct MsSet {
			bool enabled = true;
			bool transformWhenRender = true;
			const Mesh* ms = nullptr;
			const Material* mat = nullptr;
			MsSet(const Mesh* _ms, const Material* _mat);
		};
		std::string name = "nonamed node";
        Transform tf;
		glm::mat4 tf_global = glm::mat4(1); // update in render() also
		RdNode* parent = nullptr;
        std::vector<RdNode> childs;
		bool enabled = true;
		std::vector<MsSet> meshs_mats;

		RdNode(std::string_view _name, RdNode* _parent);
		RdNode(const RdNode& src);
		RdNode& operator=(const RdNode&);
		
		void addChild(std::string_view _name);
		void addMsMat(const Mesh* ms, const Material* mat);
		void clear();

		// if ModelView tf_prev to prevTf
		void updateGlobalTransform(glm::mat4 prevTf=glm::mat4(1));
		// if you want stop treversal return false;
		void treversal(std::function<bool(const Mesh* ms, const Material* mat, const glm::mat4& tf)> callback) const;
		void treversalEnabled(std::function<bool(const Mesh* ms, const Material* mat, const glm::mat4& tf)> callback) const;
	};


	class Model;
	class ModelView 
	{
	public:
		RdNode root;
		const Transform* tf_prev = nullptr;
		Animator animator;
		Model* md_data = nullptr;
		std::vector<Mesh*> own_meshes; // for delete soft body
		std::vector<MeshSkinned> skinned_meshes;

	public:
		ModelView();
		virtual ~ModelView();

		
		// before use this you must call root.updateGlobalTransform();
		glm::mat4 getLocalToMeshMtx(const Mesh* ms) const;
		glm::mat4 getLocalToBoneRootMtx() const;
        glm::vec3 getBoneWorldPos(int boneNodeIdx) const;

		// make ref with model
		ModelView(const ModelView& src);
		ModelView& operator=(const ModelView& src);
	};


	class Model: public ModelView
	{
	public:
		std::string name = "nonamed model";
		std::string path = "nodir";

		/* delete when model deleted */
		std::vector<Material*> own_materials;
		std::vector<Texture*> own_textures;
		std::vector<Mesh*> own_meshes;

		/* infos */
		GLuint total_verts = 0;
		GLuint total_tris = 0;
		glm::vec3 boundary_size = glm::vec3(1);
		glm::vec3 boundary_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 boundary_max = glm::vec3(std::numeric_limits<float>::min());
		GLuint ai_backup_flags = 0;


		/* bone */
		int nr_bones = 0;
		int depth_of_bone_root_in_rdtree = -1;
		std::map<std::string, int> bone_name_to_idx;
		std::vector<glm::mat4> bone_offsets;
        std::vector<Animation> animations;

	public:
		Model(const Model& src)		   = delete;
		Model& operator=(const Model&) = delete;

		Model(std::string_view name="nonamed");
		~Model();
		void clear();

		Material* addOwn(Material* md);
		Texture* addOwn(Texture* tex);
		Mesh* addOwn(Mesh* ms);

		void setSameMat(const Material* mat);
		void setProgToAllMat(const Program* prog);
		void setSetProgToAllMat(std::function<void(const Program&)> setProg);


		void copyFrom(const Model& src); // now for model simplification
		bool importFromFile(std::string_view modelPath, bool withAnim = false
			, bool scaleAndPivot = false, float maxSize=2.f, glm::vec3 pivot={0,-1,0});
		// render tree에서 사용하는 mesh와 material은 모두 own_에 포함되어있어야 export가능.
		bool exportToFile(size_t pIndex, std::string_view exportDir); // dir without last slash
	};
}
#endif
