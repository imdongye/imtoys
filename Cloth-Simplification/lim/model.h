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

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
		std::vector<Texture> textures_loaded; // for prevent dup texture loading
		std::vector<Mesh*> meshes;
		Program* program;
		GLuint verticesNum;
		GLuint trianglesNum;
		glm::vec3 boundary_max;
		glm::vec3 boundary_min;
	private:
		friend class ModelLoader;
		friend class ModelExporter;
		std::string directory; // for load texture
		glm::mat4 pivotMat;
		// Disable Copying and Assignment
		Model(Model const&) = delete;
		Model& operator=(Model const&) = delete;
	public:
		Model(Program* _program, const std::string& _name = "")
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

			textures_loaded = model.textures_loaded;
			position = model.position;
			scale = model.scale;
			rotation = model.rotation;
			modelMat = model.modelMat;
			directory = model.directory;
			pivotMat = model.pivotMat;
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
			for( Mesh* mesh : meshes ) {
				mesh->clear();
				delete mesh;
			}
			meshes.clear();
		}
		void draw(const Camera& camera, const Light& light)
		{
			GLuint pid;
			pid = program->use();

			setUniform(pid, "projMat", camera.projMat);
			setUniform(pid, "viewMat", camera.viewMat);
			setUniform(pid, "modelMat", modelMat);
			setUniform(pid, "cameraPos", camera.position);

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

	class ModelLoader
	{
	private:
		inline static Model* model=nullptr; // temp model
	public:
		static Model* loadFile(std::string_view _path, bool makeNormalized = true)
		{
			model = new Model(nullptr);

			const std::string path = std::string(_path);
			const size_t lastSlashPos = (path.find_last_of("/\\")==std::string::npos)?0:path.find_last_of("/\\");
			const size_t dotPos = path.find_last_of('.');
			model->name = path.substr(lastSlashPos, dotPos);
			model->directory = (lastSlashPos==0)?"": path.substr(0, lastSlashPos)+"/";

			Logger::get().log("model loading : %s\n", model->name.c_str());

			/* Assimp 설정 */
			Assimp::Importer loader;
			// aiProcess_Triangulate : 다각형이 있다면 삼각형으로
			GLuint pFrags =   aiProcess_Triangulate;
			// aiProcess_GenNormals : 노멀이 없으면 생성
			pFrags |= aiProcess_GenSmoothNormals;
			// opengl 텍스쳐 밑에서 읽는문제 or stbi_set_flip_vertically_on_load(true)
			//pFrags |= aiProcess_FlipUVs;
			pFrags |= aiProcess_CalcTangentSpace;
			// 이설정을 안하면 vert array로 중복 vert생성. 키면 shared vertex
			pFrags |= aiProcess_JoinIdenticalVertices;
			// aiProcess_SplitLargeMeshes : 큰 mesh를 작은 sub mesh로 나눠줌
			// aiProcess_OptimizeMeshes : mesh를 합쳐서 draw call을 줄인다. batching?
			const aiScene* scene = loader.ReadFile(path, pFrags);

			if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
			   || !scene->mRootNode ) {
				Logger::get().log("[error, assimp]%s\n", loader.GetErrorString());
				return new Model(nullptr);
			}
			// recursive fashion
			parseNode(scene->mRootNode, scene);


			model->updateNums();
			model->updateBoundary();
			Logger::get().log("model loaded : %s, vertices: %u\n\n", model->name.c_str()
							  , model->verticesNum);

			if( makeNormalized ) {
				model->setUnitScaleAndPivot();
				model->updateModelMat();
			}
			return model;
		}
	private:
		static std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type)
		{
			// store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			std::vector<Texture>& textures_loaded = model->textures_loaded;
			std::vector<Texture> textures;
			for( GLuint i=0; i<mat->GetTextureCount(type); i++ ) {
				aiString ai_str;
				/* blend 등 attrib에서 bm값이 안읽힘 */
				mat->GetTexture(type, i, &ai_str);
				std::string str_path(ai_str.C_Str());
				/* assimp가 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) */
				str_path = str_path.substr(0, str_path.find_first_of(' '));

				// check already loaded
				// to skip loading same texture 
				bool skip = false;
				for( GLuint j=0; j<textures_loaded.size(); j++ ) {
					if( textures_loaded[j].path == str_path ) {
						textures.push_back(textures_loaded[j]);
						skip = true;
						break;
					}
				}
				// load texture
				if( !skip ) {
					Texture texture;
					std::string fullTexPath = model->directory+str_path;

					texture.id = loadTextureFromFile(fullTexPath, true);

					switch( type ) {
					case aiTextureType_DIFFUSE: texture.type = "map_Kd"; break;
					case aiTextureType_SPECULAR: texture.type = "map_Ks"; break;
					case aiTextureType_AMBIENT: texture.type = "map_Ka"; break;
					case aiTextureType_HEIGHT: texture.type = "map_Bump"; break;
					}
					texture.path = str_path;
					textures.push_back(texture);
					textures_loaded.push_back(texture);
				}
			}
			return textures;
		}
		static Mesh* getParsedMesh(aiMesh* mesh, const aiScene* scene)
		{
			std::vector<n_mesh::Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<Texture> textures;
			// now only triangle mesh
			const GLuint angles = 3;

			// - per vertex
			n_mesh::Vertex vertex;
			for( GLuint i=0; i<mesh->mNumVertices; i++ ) {
				vertex.p = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				if( mesh->HasNormals() ) {
					vertex.n = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}
				// 버텍스당 최대 8개의 uv를 가질수있지만 하나만 사용.
				if( mesh->mTextureCoords[0] ) {
					vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
					// tangent
					vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
					// bitangent
					vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
				}
				else {
					vertex.uv = glm::vec2(0.0f);
				}
				vertices.push_back(vertex);
			}

			// - per triangles
			for( GLuint i=0; i<mesh->mNumFaces; i++ ) {
				aiFace& face = mesh->mFaces[i];
				// if not triangle mesh
				if( face.mNumIndices != angles )
					return new Mesh();
				for( GLuint j=0; j<face.mNumIndices; j++ )
					indices.push_back(face.mIndices[j]);
			}

			// - materials. per texture type
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			// 1. diffuse maps
			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. specular maps
			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			// 3. normal maps
			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT);
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			// 4. ambient maps
			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT);
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

			Mesh* newMesh = new Mesh(vertices, indices, textures, mesh->mName.C_Str(), angles);
			aiColor3D ai_color;
			material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color);
			newMesh->color = glm::vec3(ai_color.r, ai_color.g, ai_color.b);
			return newMesh;
		}
		static void parseNode(aiNode* node, const aiScene* scene, int depth = 0)
		{
			// in current node
			for( GLuint i=0; i<node->mNumMeshes; i++ ) {
				model->meshes.push_back(getParsedMesh(scene->mMeshes[node->mMeshes[i]], scene));
				for( int j=0; j<depth; j++ ) Logger::get()<<'\t';
				Logger::get().log("mesh loaded : %s,", node->mName.C_Str());
				(*(model->meshes.back())).print();
			}
			for( GLuint i=0; i<node->mNumChildren; i++ ) {
				parseNode(node->mChildren[i], scene, depth+1);
			}
		}
	};

	class ModelExporter
	{
	public:
		static void exportObj(const char* path)
		{
			aiScene* scene = new aiScene();
			aiMesh* mesh = new aiMesh();
			//mesh->mPrimitiveTypes =AI_PRIMITIVE_TYPE;

			Assimp::Exporter exporter;
			const aiExportFormatDesc* format = exporter.GetExportFormatDescription(0);
			aiReturn ret = exporter.Export(scene, format->id, path, scene->mFlags);
			Logger::get()<<"[error::exporter]"<<exporter.GetErrorString()<<Logger::endl;
		}
	};
}
#endif
