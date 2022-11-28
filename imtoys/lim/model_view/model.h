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

#ifndef MODEL_H
#define MODEL_H

namespace lim
{
	class Model
	{
	public:
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		glm::mat4 model_mat;
		std::string name;
		std::vector<Texture> textures_loaded; // for prevent dup texture loading
		std::vector<Mesh*> meshes;
		Program* program;
		float bumpHeight=100;
		float texDelta = 0.00001;

		GLuint nr_vertices;
		GLuint nr_triangles;
		glm::vec3 boundary_max;
		glm::vec3 boundary_min;
	private:
		std::string directory; // for load texture
		glm::mat4 pivot_mat;
		// From : https://www.ostack.cn/qa/?qa=834163/
		friend class ModelLoader;
		friend class ModelExporter;
		GLuint ai_nr_mats;
		void** ai_mats;
	private:
		// Disable Copying and Assignment
		Model(Model const&) = delete;
		Model& operator=(Model const&) = delete;
	public:
		Model(Program* _program=nullptr, const std::string& _name = "")
			: position(glm::vec3(0)), rotation(glm::quat()), scale(glm::vec3(1)),
			pivot_mat(glm::mat4(1.0f)), nr_vertices(0), name(_name), program(_program) // mat4(1) is identity mat
		{
			updateModelMat();
		}
		// copy with new mesh
		Model(const Model& model, const std::vector<Mesh*>& _meshes)
			:Model(model.program, model.name)
		{
			meshes = _meshes;
			updateNums();
			updateBoundary();

			position = model.position;
			rotation = model.rotation;
			scale = model.scale;
			model_mat = model.model_mat;
			textures_loaded = model.textures_loaded; // deepcopy
			directory = model.directory;
			pivot_mat = model.pivot_mat;
			ai_nr_mats = model.ai_nr_mats;
			ai_mats = model.ai_mats; // shared memory
		}
		// create with only mesh
		Model(Mesh* _mesh, Program* _program, std::string _name ="")
			:Model(_program, _name)
		{
			meshes.push_back(_mesh);

			updateNums();
			updateBoundary();
			Logger::get().log("model(mesh) generaged : %s, vertices: %u\n\n", name.c_str(), nr_vertices);
		}
		~Model()
		{
			clear();
		}
	public:
		void clear()
		{
			for( Texture& tex : textures_loaded ) {
				tex.clear();
			}
			for( Mesh* mesh : meshes ) {
				delete mesh;
			}
			meshes.clear();
		}
		void draw(const Camera& camera, const Light& light)
		{
			if( program==nullptr )
				Logger::get()<<"[error] model has not progam"<<Logger::endl;
			GLuint pid = program->use();

			setUniform(pid, "projMat", camera.proj_mat);
			setUniform(pid, "viewMat", camera.view_mat);
			setUniform(pid, "modelMat", model_mat);
			setUniform(pid, "cameraPos", camera.position);
			setUniform(pid, "bumpHeight", bumpHeight);
			setUniform(pid, "texDelta", texDelta);

			light.setUniforms(pid);

			for( GLuint i=0; i<meshes.size(); i++ )
				meshes[i]->draw(pid, textures_loaded);
		}
		void updateModelMat()
		{
			glm::mat4 translateMat = glm::translate(position);
			glm::mat4 scaleMat = glm::scale(scale);
			glm::mat4 rotateMat = glm::toMat4(rotation);
			model_mat = translateMat * rotateMat * scaleMat * pivot_mat;
		}
		void setUnitScaleAndPivot()
		{
			glm::vec3 bSize = getBoundarySize();
			setPivot(boundary_min + bSize*0.5f);

			const float unit_length = 2.f;
			float max_axis_length = glm::max(bSize.x, glm::max(bSize.y, bSize.z));
			scale = glm::vec3(unit_length/max_axis_length);

			position = glm::vec3(0, scale.y*bSize.y*0.5f, 0);
		}
		glm::vec3 getBoundarySize()
		{
			return boundary_max-boundary_min;
		}
		void setPivot(glm::vec3 pivot)
		{
			Logger::get()<<"pivot: "<<glm::to_string(pivot)<<Logger::endl;
			pivot_mat = glm::translate(-pivot);
		}/*
		void reloadNormalMap(std::string_view fullpath)
		{
			bool hasBumpMap = false;
			for( Texture& tex : textures_loaded ) {
				if( tex.type == "map_Bump" ) {
					glDeleteTextures(0, &tex.id);
					tex.id = loadTextureFromFile(fullpath, false);
					hasBumpMap = true;
					break;
				}
			}
			if( hasBumpMap==false ) {
				Texture newTex;
				std::string fpth(fullpath);
				newTex.id = loadTextureFromFile(fullpath, false);
				newTex.path = fpth.substr(fpth.find_last_of('/')+1);
				newTex.type = "map_Bump";
				textures_loaded.push_back(newTex);
			}
		}*/
	private:
		void updateNums()
		{
			nr_vertices = 0;
			nr_triangles = 0;
			for( Mesh* mesh : meshes ) {
				nr_vertices += mesh->vertices.size();
				nr_triangles += mesh->indices.size()/3;
			}
		}
		void updateBoundary()
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
			Logger::get()<<"boundary size: "<<glm::to_string(getBoundarySize())<<Logger::endl;
		}
	};
}
#endif
