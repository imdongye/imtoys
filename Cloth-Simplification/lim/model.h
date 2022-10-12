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
		glm::mat4 modelMat;
		std::string name;
		std::vector<Texture*> textures_loaded; // for prevent dup texture loading
		std::vector<Mesh*> meshes;
		Program* program;
		float bumpHeight=100;

		GLuint verticesNum;
		GLuint trianglesNum;
		glm::vec3 boundary_max;
		glm::vec3 boundary_min;
	private:
		std::string directory; // for load texture
		glm::mat4 pivotMat;
		// From : https://www.ostack.cn/qa/?qa=834163/
		friend class ModelLoader;
		friend class ModelExporter;
		GLuint aiNumMats;
		void** aiMats;
	private:
		// Disable Copying and Assignment
		Model(Model const&) = delete;
		Model& operator=(Model const&) = delete;
	public:
		Model(Program* _program=nullptr, const std::string& _name = "")
			: position(glm::vec3(0)), rotation(glm::quat()), scale(glm::vec3(1)),
			pivotMat(glm::mat4(1.0f)), verticesNum(0), name(_name), program(_program) // mat4(1) is identity mat
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
			modelMat = model.modelMat;
			textures_loaded = model.textures_loaded; // deepcopy
			directory = model.directory;
			pivotMat = model.pivotMat;
			aiNumMats = model.aiNumMats;
			aiMats = model.aiMats; // shared memory
		}
		// create with only mesh
		Model(Mesh* _mesh, Program* _program, std::string _name ="")
			:Model(_program, _name)
		{
			meshes.push_back(_mesh);

			updateNums();
			updateBoundary();
			Logger::get().log("model(mesh) generaged : %s, vertices: %u\n\n", name.c_str(), verticesNum);
		}
		~Model()
		{
			clear();
		}
	public:
		void clear()
		{
			for( Texture* tex : textures_loaded ) {
				glDeleteTextures(0, &(tex->id));
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

			setUniform(pid, "projMat", camera.projMat);
			setUniform(pid, "viewMat", camera.viewMat);
			setUniform(pid, "modelMat", modelMat);
			setUniform(pid, "cameraPos", camera.position);
			setUniform(pid, "bumpHeight", bumpHeight);

			light.setUniforms(pid);

			for( GLuint i=0; i<meshes.size(); i++ )
				meshes[i]->draw(pid);
		}
		void updateModelMat()
		{
			glm::mat4 translateMat = glm::translate(position);
			glm::mat4 scaleMat = glm::scale(scale);
			glm::mat4 rotateMat = glm::toMat4(rotation);
			modelMat = translateMat * rotateMat * scaleMat * pivotMat;
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
			pivotMat = glm::translate(-pivot);
		}
		void reloadNormalMap(std::string_view fullpath)
		{
			for( Texture* tex : textures_loaded ) {
				if( tex->type == "map_Bump" ) {
					glDeleteTextures(0, &tex->id);
					tex->id = loadTextureFromFile(fullpath, false);
				}
			}
		}
	private:
		void updateNums()
		{
			verticesNum = 0;
			trianglesNum = 0;
			for( Mesh* mesh : meshes ) {
				verticesNum += mesh->vertices.size();
				trianglesNum += mesh->indices.size()/3;
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
