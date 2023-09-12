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

namespace lim
{
	class Model
	{
	public:
		struct Node
		{
			std::vector<Mesh*> meshes;
			std::vector<Node*> childs;
			glm::mat4 trans = glm::mat4(1);
			~Node();
		};
	public:
		std::string name = "nonamed model";
		std::string path = "nopath";

		/* transformation */
		glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat();
		glm::vec3 scale = glm::vec3(1);
		glm::mat4 pivot_mat = glm::mat4(1);
		glm::mat4 model_mat = glm::mat4(1); // = trans*rot*scale*pivot

		/* render data */
		std::vector<Material*> materials;
		std::vector<TexBase*> textures_loaded;
		std::vector<Mesh*> meshes;
		Node* root = nullptr;

		GLuint nr_vertices = 0;
		GLuint nr_triangles = 0;
		glm::vec3 boundary_max = glm::vec3(-1);
		glm::vec3 boundary_min = glm::vec3(-1);
		glm::vec3 boundary_size = glm::vec3(-1);
		float pivoted_scaled_bottom_height = 0;
		
	private:
		/* backup for export */
		bool isCloned = false;
		aiScene* backupScene = nullptr;
		friend Model *importModelFromFile(std::string_view modelPath, bool normalizeAndPivot, bool withMaterial, bool readyExport);
		friend void exportModelToFile(std::string exportDir, Model *model, size_t pIndex);
	
	private:
		// Disable Copying and Assignment
		Model(Model const &) = delete;
		Model &operator=(Model const &) = delete;

	public:
		Model();
		~Model();
		// 부모 Model에 material, texture, backup 이 있으므로 부모를 삭제하면 안됨.
		// mesh정보만 clone됨.
		Model* clone();
		void updateModelMat();
		void updateUnitScaleAndPivot();
		void updateNrAndBoundary();
		void setPivot(const glm::vec3& pivot);
	};


	// import model
	Model* importModelFromFile(std::string_view modelPath, bool unitScaleAndPivot = false, bool withMaterial = true, bool readyExport=false);

	// export model
	int getNrExportFormats();
	const aiExportFormatDesc *getExportFormatInfo(int idx);
	void exportModelToFile(std::string_view exportDir, Model *model, size_t pIndex);
}
#endif
