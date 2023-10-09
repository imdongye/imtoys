/*

2023-09-11 / im dong ye
edit learnopengl code

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
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../texture.h"
#include "../program.h"
#include "mesh.h"
#include "light.h"
#include "camera.h"
#include <assimp/scene.h>
#include <assimp/cexport.h>
#include <assimp/cimport.h>
#include <limits>

namespace lim
{
	// clone able
	class Model
	{
	public:
		struct Node
		{
		private:
			int nr_meshes=0;
			std::vector<Mesh*> meshes;
			std::vector<Material*> mats;
		public:
			void addMesh(Mesh* ms, Material* mat = nullptr) {
				meshes.push_back(ms);
				mats.push_back(mat);
				nr_meshes++;
			}
			std::pair<const Mesh*, const Material*> getMesh(int idx) const {
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
			glm::mat4 transformation = glm::mat4(1);
		};
	public:
		std::string name = "nonamed model";
		std::string path = "nodir";

		/* transformation */
		glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat();
		glm::vec3 scale = glm::vec3(1);
		glm::mat4 pivot_mat = glm::mat4(1);
		glm::mat4 model_mat = glm::mat4(1); // = trans*rot*scale*pivot

		/* render data */
		Node root;
		Material* default_material;
		
		/* delete when model deleted */
		std::vector<Material*> my_materials;
		std::vector<Texture*> my_textures;
		std::vector<Mesh*> my_meshes;

		GLuint nr_vertices = 0;
		GLuint nr_triangles = 0;
		glm::vec3 boundary_size = glm::vec3(1);
		glm::vec3 boundary_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 boundary_max = glm::vec3(std::numeric_limits<float>::min());
		float pivoted_scaled_bottom_height = 0;
		
		GLuint ai_backup_flags = 0;
	private:
		Model& operator=(const Model&) = delete;
	public:
		Model(std::string_view name="nonamed");
		Model(const Model& src);
		Model(Model&& src, bool makeRef=false) noexcept;
		Model& operator=(Model&& src) noexcept;
		~Model() noexcept;
		void releaseResource();
		
		void updateModelMat();
		void updateUnitScaleAndPivot();
		void updateNrAndBoundary();
		void setPivot(const glm::vec3& pivot);

		bool importFromFile(std::string_view modelPath, bool unitScaleAndPivot = false, bool withMaterial = true);
		bool exportToFile(size_t pIndex, std::string_view exportPath);
	};

	// get format data
	int getNrImportFormats();
	const char* getImportFormat(int idx);
	std::string findModelInDirectory(std::string_view path);
	int getNrExportFormats();
	const aiExportFormatDesc* getExportFormatInfo(int idx);
}
#endif
