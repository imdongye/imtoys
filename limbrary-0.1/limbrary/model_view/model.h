//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
// texture uniform sampler2d variable name rule
// map_Kd0, map_Kd1 ...
//
//  TODO list:
//  1. export
//  2. rigging
//  3. not gl_static으로 실시간 vert변화
//  4. width, height, depth 찾아서 -1~1공간으로 scaling
//  5. load model 이 모델안에 있는데 따로 빼야될까
//  6. 언제 어디서 업데이트해줘야하는지 규칙정하기
//	7. reload normal map 외부로 빼기
//
//

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
#include <assimp/cexport.h>

namespace lim
{
	class Model
	{
	public:
		std::string name = "nonamed";
		glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat();
		glm::vec3 scale = glm::vec3(1);
		glm::mat4 model_mat = glm::mat4(1); // = trans*rot*scale*pivot
		// shared_ptr for managing omit dup texture loading
		std::vector<std::shared_ptr<Texture>> textures_loaded;
		std::vector<Mesh *> meshes;

		Program* program;
		float bumpHeight = 100;
		float texDelta = 0.00001f;

		GLuint nr_vertices = 0;
		GLuint nr_triangles = 0;
		glm::vec3 boundary_max;
		glm::vec3 boundary_min;

	public:
		// define when model loading
		glm::mat4 pivot_mat = glm::mat4(1);
		// for texture loading
		std::string data_dir;
		// for model exporting
		GLuint ai_nr_mats;
		void **ai_mats;

	private:
		// Disable Copying and Assignment
		Model(Model const &) = delete;
		Model &operator=(Model const &) = delete;

	public:
		Model(const std::string_view _name = "", Program *_program = nullptr);
		// copy with new mesh
		Model(const Model &model, const std::vector<Mesh *> &_meshes);
		// create with only mesh
		Model(Mesh *_mesh, std::string_view _name = "none", Program *_program = nullptr);
		~Model();
		void clear();
		void updateModelMat();
		void setUnitScaleAndPivot();
		glm::vec3 getBoundarySize();
		void setPivot(glm::vec3 pivot);

	public:
		void updateNums();
		void updateBoundary();
	};

	// import model
	Model* importModelFromFile(std::string_view path, bool makeNormalized = false);

	// export model
	int getNrExportFormats();
	const aiExportFormatDesc *getExportFormatInfo(int idx);
	void exportModelToFile(std::string_view exportDir, Model *model, size_t pIndex);
}
#endif
