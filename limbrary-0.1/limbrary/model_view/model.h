/*

2023-09-11 / im dong ye
edit learnopengl code

Note:
default_mat을 수정한다면 program 과 set_prog는 반드시 지정해줘야함.

Transform 하이라키 구조는 굳이 필요하지 않고 Model 의 RenderTree 를 사용해도된다.

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
	class Model : public Transform
	{
	public:
		struct Node
		{
		private:
			int nr_meshes=0;
			std::vector<const Mesh*> meshes;
			std::vector<const Material*> mats;
		public:
			// if mat is null then mesh rendered with past material
			void addMeshWithMat(const Mesh* ms, const Material* mat = nullptr) {
				meshes.push_back(ms);
				mats.push_back(mat);
				nr_meshes++;
			}
			std::pair<const Mesh*, const Material*> getMeshWithMat(int idx) const {
				return std::make_pair(meshes[idx], mats[idx]);
			}
			int getNrMesh() const {
				return nr_meshes;
			}
			void treversal(std::function<void(const Mesh* ms, const Material* mat)> callback) const {
				for( int i=0; i<nr_meshes; i++ ) {
					callback(meshes[i], mats[i]);
				}
				for( const Model::Node& child : childs ) {
					child.treversal(callback);
				}
			}
			std::vector<Node> childs;
			glm::mat4 transform = glm::mat4(1);
		};
	public:
		std::string name = "nonamed model";
		std::string path = "nodir";

		/* transformation */
		Transform normalize_term;
		glm::mat4 model_mat;

		/* render data */
		Node root;
		Material* default_material;
		
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
		void updateModelMat();
		void updateUnitScaleAndPivot();
		void updateNrAndBoundary();

		bool importFromFile(std::string_view modelPath, bool unitScaleAndPivot = false, bool withMaterial = true);
		// render tree에서 사용하는 mesh와 material은 모두 my_에 포함되어있어야 export가능.
		bool exportToFile(size_t pIndex, std::string_view exportDir); // dir without last slash

		void releaseResource();
	public:
		Model(std::string_view name="nonamed");
		Model(const Model& src, bool makeRef=false);
		Model(Model&& src) noexcept;
		Model& operator=(Model&& src) noexcept;
		~Model() noexcept;
	private:
		Model& operator=(const Model&) = delete;
	};
}
#endif
