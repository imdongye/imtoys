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

*/

#ifndef __model_h_
#define __model_h_

#include <string>
#include <vector>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../texture.h"
#include "mesh.h"
#include "camera.h"
#include "transform.h"

namespace lim
{
	// clone able
	class Model
	{
	public:
		struct Node
		{
		public:
			Transform transform;
			// if mat is null then mesh rendered with past material
			std::vector<std::pair<const Mesh*, const Material*>> meshs_mats;
			std::vector<Node> childs;

			void treversal(std::function<void(const Mesh* ms, const Material* mat, const glm::mat4& transform)> callback
				, const glm::mat4& topTransform = glm::mat4(1) ) const 
			{
				glm::mat4 curTransform = transform.mat * topTransform;
				for( const auto& meshmat : meshs_mats ) {
					callback(meshmat.first, meshmat.second, curTransform);
				}
				for( const Model::Node& child : childs ) {
					child.treversal(callback, curTransform);
				}
			}
		};

	public:
		std::string name = "nonamed model";
		std::string path = "nodir";

		Node root;
		Transform* tf_normalize = nullptr;
		Transform* tf = nullptr;
		
		/* delete when model deleted */
		std::vector<Material*> my_materials;
		std::vector<Texture*> my_textures;
		std::vector<Mesh*> my_meshes;

		/* infos */
		GLuint nr_vertices = 0;
		GLuint nr_triangles = 0;
		glm::vec3 boundary_size = glm::vec3(1);
		glm::vec3 boundary_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 boundary_max = glm::vec3(std::numeric_limits<float>::min());
		float pivoted_scaled_bottom_height = 0;
		GLuint ai_backup_flags = 0;

	public:
		Model(Model&&)			       = delete;
		Model& operator=(const Model&) = delete;
		Model& operator=(Model&&)      = delete;

		Model(std::string_view name="nonamed");
		Model(const Model& src, bool makeRef=false);
		virtual ~Model();

		void updateNrAndBoundary();
		void updateUnitScaleAndPivot();

		bool importFromFile(std::string_view modelPath, bool unitScaleAndPivot = false, bool withMaterial = true);
		// render tree에서 사용하는 mesh와 material은 모두 my_에 포함되어있어야 export가능.
		bool exportToFile(size_t pIndex, std::string_view exportDir); // dir without last slash

		void releaseResource();
	};
}
#endif
