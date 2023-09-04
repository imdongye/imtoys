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
#include <limbrary/model_view/model.h>
#include <limbrary/log.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

namespace lim
{
	Model::Model(const std::string_view _name, Program* _program)
		: name(_name), program(_program)
	{
		updateModelMat();
	}
	// copy with new mesh
	Model::Model(const Model& model, const std::vector<Mesh*>& _meshes)
		:Model(model.name, model.program)
	{
		meshes = _meshes;
		updateNums();
		updateBoundary();

		position = model.position;
		orientation = model.orientation;
		scale = model.scale;
		model_mat = model.model_mat;
		textures_loaded = model.textures_loaded; // deepcopy
		data_dir = model.data_dir;
		pivot_mat = model.pivot_mat;
		ai_nr_mats = model.ai_nr_mats;
		ai_mats = model.ai_mats; // shared memory
	}
	// create with only mesh
	Model::Model(Mesh* _mesh, std::string_view _name, Program* _program)
		:Model(_name, _program)
	{
		meshes.push_back(_mesh);
		updateNums();
		updateBoundary();
		log::info("model(mesh) generaged : %s, vertices: %u\n\n", name.c_str(), nr_vertices);
	}
	Model::~Model()
	{
		clear();
	}
	void Model::clear()
	{
		textures_loaded.clear();
		for( Mesh* mesh : meshes ) {
			delete mesh;
		}
		meshes.clear();
	}
	void Model::draw(const GLuint pid)
	{
		if( pid==0 )
			log::err("model draw pid param is zero\n");

		setUniform(pid, "modelMat", model_mat);
		setUniform(pid, "bumpHeight", bumpHeight);
		setUniform(pid, "texDelta", texDelta);

		for( GLuint i=0; i<meshes.size(); i++ )
			meshes[i]->draw(pid);
	}
	void Model::draw(const Camera& camera, const Light& light)
	{
		if( program==nullptr )
			log::err("model has not progam\n");
		GLuint pid = program->use();

		setUniform(pid, "projMat", camera.proj_mat);
		setUniform(pid, "viewMat", camera.view_mat);
		setUniform(pid, "modelMat", model_mat);
		setUniform(pid, "cameraPos", camera.position);
		setUniform(pid, "bumpHeight", bumpHeight);
		setUniform(pid, "texDelta", texDelta);

		light.setUniforms(*program);

		for( GLuint i=0; i<meshes.size(); i++ )
			meshes[i]->draw(pid);
	}
	void Model::updateModelMat()
	{
		glm::mat4 translateMat = glm::translate(position);
		glm::mat4 scaleMat = glm::scale(scale);
		glm::mat4 rotateMat = glm::toMat4(orientation);
		model_mat = translateMat * rotateMat * scaleMat * pivot_mat;
	}
	void Model::setUnitScaleAndPivot()
	{
		glm::vec3 bSize = getBoundarySize();
		setPivot(boundary_min + bSize*0.5f);

		const float unit_length = 2.f;
		float max_axis_length = std::max(bSize.x, std::max(bSize.y, bSize.z));
		scale = glm::vec3(unit_length/max_axis_length);

		position = glm::vec3(0, scale.y*bSize.y*0.5f, 0);
	}
	glm::vec3 Model::getBoundarySize()
	{
		return boundary_max-boundary_min;
	}
	void Model::setPivot(glm::vec3 pivot)
	{
		log::info("pivot: %s\n",glm::to_string(pivot).c_str());
		pivot_mat = glm::translate(-pivot);
	}
	void Model::updateNums()
	{
		nr_vertices = 0;
		nr_triangles = 0;
		for( Mesh* mesh : meshes ) {
			nr_vertices += mesh->vertices.size();
			nr_triangles += mesh->indices.size()/3;
		}
	}
	void Model::updateBoundary()
	{
		if( meshes.size()==0 )
			return;
		boundary_max = meshes[0]->vertices[0].p;
		boundary_min = boundary_max;

		for( Mesh* mesh : meshes ) {
			for( n_mesh::Vertex& v : mesh->vertices ) {
				if( boundary_max.x < v.p.x ) boundary_max.x = v.p.x;
				else if( boundary_min.x > v.p.x ) boundary_min.x = v.p.x;

				if( boundary_max.y < v.p.y ) boundary_max.y = v.p.y;
				else if( boundary_min.y > v.p.y ) boundary_min.y = v.p.y;

				if( boundary_max.z < v.p.z ) boundary_max.z = v.p.z;
				else if( boundary_min.z > v.p.z ) boundary_min.z = v.p.z;
			}
		}
		log::info("boundary size: %s\n", glm::to_string(getBoundarySize()).c_str());
	}
}
