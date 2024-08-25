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
#include "../containers/own_ptr.h"

namespace lim
{
	struct RdNode {
		std::string name = "nonamed node";
        Transform tf;
		glm::mat4 mtx_global = glm::mat4(1); // update in render() also
		const RdNode* parent = nullptr;
        std::vector<RdNode> childs;
		
		bool enabled = true;
		bool visible = true;
		bool is_identity_mtx = false;
		bool is_local_is_global = true;

		const Mesh* ms = nullptr;
		const Material* mat = nullptr;


		RdNode(std::string_view _name, const RdNode* _parent, const Mesh* _ms, const Material* _mat);
		RdNode(const RdNode& src);
		RdNode& operator=(const RdNode&);
		RdNode(RdNode&& src) noexcept;
		RdNode& operator=(RdNode&& src) noexcept;

		void clear() noexcept;
		
		void addChild(std::string_view _name, const Mesh* _ms, const Material* _mat);

		// if ModelView tf_prev to prevTf
		void updateGlobalTransform(glm::mat4 prevTf=glm::mat4(1));

		void dfsRender(std::function<void(const Mesh* ms, const Material* mat, const glm::mat4& mtxGlobal)> callback) const;

		// if you want stop treversal return false;
		void dfsAll(std::function<bool(RdNode& nd)> callback);
		void dfsAll(std::function<bool(const RdNode& nd)> callback) const;
	};


	struct Model;
	struct ModelView 
	{
		RdNode root;
		OwnPtr<Animator> own_animator = nullptr; // own
		std::vector<OwnPtr<Mesh>> own_meshes; // for delete soft body & skinned mesh
		std::vector<RdNode*> skinned_mesh_nodes; // for update skinned own mesh buffer

		const Transform* tf_prev = nullptr;
		const Model* md_data = nullptr;


		ModelView();
		virtual ~ModelView();
		// make ref with model
		ModelView(const ModelView& src);
		ModelView& operator=(const ModelView& src);
		ModelView(ModelView&& src) = delete;
		ModelView& operator=(ModelView&& src) = delete;

		void clear() noexcept;

		// before use this you must call root.updateGlobalTransform();
		glm::mat4 getLocalToMeshMtx(const Mesh* ms) const;
		glm::mat4 getLocalToBoneRootMtx() const;
        glm::vec3 getBoneWorldPos(int boneNodeIdx) const;
	};


	struct Model: public ModelView
	{
		std::string name = "nonamed model";
		std::string path = "nodir";

		/* delete when model deleted */
		std::vector<OwnPtr<Material>> own_materials;
		std::vector<OwnPtr<Texture>> own_textures;
		std::vector<OwnPtr<Mesh>> own_meshes;

		/* infos */
		GLuint total_verts = 0;
		GLuint total_tris = 0;
		glm::vec3 boundary_size = glm::vec3(1);
		glm::vec3 boundary_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 boundary_max = glm::vec3(std::numeric_limits<float>::min());
		GLuint ai_backup_flags = 0;


		/* bone */
		int depth_of_bone_root_in_rdtree = -1;
		int nr_weighted_bones = 0;
		std::map<std::string, int> name_to_weighted_bone_idx;
		std::vector<glm::mat4> weighted_bone_offsets;
        std::vector<Animation> animations;



		Model(const Model& src)		   = delete;
		Model& operator=(const Model&) = delete;

		Model(std::string_view name="nonamed");
		~Model();
		void clear() noexcept;

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
