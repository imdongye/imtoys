//
//  2022-09-27 / im dong ye
//  edit learnopengl code
// 
//	Assimp는 namespace도 없고 자료형의 이름이 camelCase이므로
//	Assimp에 한해서 지역변수 이름을 snake_case로 작성한다.
// 
//  Todo:
//  1. model path texture path 그냥 하나로 합치기
//


#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace lim
{
	class ModelLoader
	{
	private:
		inline static Model* model=nullptr; // temp model
	public:
		static Model* loadFile(std::string_view _path, bool makeNormalized = false)
		{
			std::string path(_path);

			model = new Model();

			// 찾지 못하면 std::string:npos == -1
			const size_t lastSlashPos = path.find_last_of("/\\");
			const size_t dotPos = path.find_last_of('.');
			model->name = path.substr(lastSlashPos+1, dotPos-lastSlashPos-1);
			model->data_dir = (lastSlashPos==0) ? "" : path.substr(0, lastSlashPos)+"/";

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

			if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) {
				Logger::get()<<loader.GetErrorString();
				return nullptr;
			}

			/* backup for export */
			GLuint nr_mat = scene->mNumMaterials;
			model->ai_nr_mats = nr_mat;
			model->ai_mats = (void**)(new aiMaterial*[nr_mat]);

			for( int i=0; i<nr_mat; i++ ) {
				model->ai_mats[i] = new aiMaterial();
				aiMaterial::CopyPropertyList((aiMaterial*)(model->ai_mats[i]), scene->mMaterials[i]);
			}
			//aimat->AddProperty()

			/* load Mesh */
			parseNode(scene->mRootNode, scene);
			
			model->updateNums();
			model->updateBoundary();
			Logger::get().log("model loaded : %s, vertices: %u\n\n", model->name.c_str()
							  , model->nr_vertices);

			if( makeNormalized ) {
				model->setUnitScaleAndPivot();
				model->updateModelMat();
			}

			return model;
		}
	private:
		static std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* ai_mat, aiTextureType ai_type)
		{
			// store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			std::vector<std::shared_ptr<Texture>>& textures_loaded = model->textures_loaded;
			std::vector<std::shared_ptr<Texture>> meshTextures;

			for( GLuint i=0; i<ai_mat->GetTextureCount(ai_type); i++ ) {
				aiString ai_path;
				/* blend 등 attrib에서 bm값이 안읽힘 */
				ai_mat->GetTexture(ai_type, i, &ai_path);
				std::string texPath(ai_path.C_Str());
				// assimp가 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4)
				// 그래서 아래와 같이 필터링한다.
				texPath = texPath.substr(0, texPath.find_first_of(' '));
				texPath = model->data_dir + texPath;

				// check already loaded
				// to skip loading same texture 
				bool skip = false;
				for( GLuint j=0; j< textures_loaded.size(); j++ ) {
					if( texPath.compare(textures_loaded[j]->path)==0 ) {
						meshTextures.push_back(textures_loaded[j]);
						skip = true;
						break;
					}
				}
				
				// load texture
				if( !skip ) {
					// kd일때만 linear space변환
					auto texture = std::make_shared<Texture>(texPath, (ai_type==aiTextureType_DIFFUSE)?GL_SRGB8:GL_RGB8);
					texture->internal_model_path = texture->path.c_str()+model->data_dir.size();
					Logger::get().log("%s\n", texture->internal_model_path);

					switch( ai_type ) {
                        case aiTextureType_DIFFUSE: texture->tag = "map_Kd"; break;
                        case aiTextureType_SPECULAR: texture->tag = "map_Ks"; break;
                        case aiTextureType_AMBIENT: texture->tag = "map_Normal"; break;
                        case aiTextureType_HEIGHT: texture->tag = "map_Bump"; break; // map_bump, bump
                        default: break;
                    }
					Logger::get()<<"-"<<texture->tag<<Logger::endl;
					textures_loaded.push_back(texture);
					meshTextures.push_back(textures_loaded.back());
				}
			}
			return meshTextures;
		}
		static Mesh* getParsedMesh(aiMesh* mesh, const aiScene* scene)
		{
			std::vector<n_mesh::Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<std::shared_ptr<Texture>> textures;
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
					return nullptr;
				for( GLuint j=0; j<face.mNumIndices; j++ )
					indices.push_back(face.mIndices[j]);
			}
			// - materials. per texture type
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			// 1. diffuse maps
			auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			// 2. specular maps
			auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			// 3. normal maps
			auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT);
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			// 4. ambient maps
			auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT);
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

			Mesh* newMesh = new Mesh(vertices, indices, textures, mesh->mName.C_Str());
			aiColor3D ai_color;
			material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color);
			newMesh->color = glm::vec3(ai_color.r, ai_color.g, ai_color.b);
			newMesh->ai_mat_idx = mesh->mMaterialIndex;
			return newMesh;
		}
		static void parseNode(aiNode* node, const aiScene* scene, int depth_tex = 0)
		{
			// in current node
			for( GLuint i=0; i<node->mNumMeshes; i++ ) {
				model->meshes.push_back(getParsedMesh(scene->mMeshes[node->mMeshes[i]], scene));
				for( int j=0; j<depth_tex; j++ ) Logger::get()<<" ";
				Logger::get().log("mesh loaded : %s,", node->mName.C_Str());
				(*(model->meshes.back())).print();
			}
			for( GLuint i=0; i<node->mNumChildren; i++ ) {
				parseNode(node->mChildren[i], scene, depth_tex+1);
			}
		}
	};
}

#endif
