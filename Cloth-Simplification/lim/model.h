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

#ifndef MODEL_H
#define MODEL_H

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>
#include <functional>
#include <vector>

#include "program.h"

namespace lim
{
	const int MAX_BONE_INFLUENCE = 4;

	namespace n_model
	{
		// 4byte * (3+3+2+3+3+4+4) = 88
		// offsetof(Vertex, Normal) = 12
		struct Vertex
		{
			glm::vec3 p, n;
			glm::vec2 uv;
			glm::vec3 tangent, bitangent;
			int m_BoneIDs[MAX_BONE_INFLUENCE];
			float m_Weights[MAX_BONE_INFLUENCE];

			Vertex& operator=(const Vertex& copy)
			{
				p=copy.p; n=copy.n;
				uv=copy.uv;
				tangent=copy.tangent; bitangent = copy.bitangent;
				memcpy(m_BoneIDs, copy.m_BoneIDs, sizeof(int) * MAX_BONE_INFLUENCE);
				memcpy(m_Weights, copy.m_Weights, sizeof(float) * MAX_BONE_INFLUENCE);
				return *this;

			}
		};
	} // namespace n_model

	struct Texture
	{
		GLuint id;
		std::string type;
		std::string path; // relative path+filename or only filename
	};

	GLuint loadTextureFromFile(const char* cpath, bool toLinear = true)
	{
		std::string spath = std::string(cpath);

		GLuint texID=0;
		glGenTextures(1, &texID);

		int w, h, channels;
		// 0 => comp 있는대로
		void* buf = stbi_load(cpath, &w, &h, &channels, 4); // todo: 0

		if( !buf )
		{
			fprintf(stderr, "texture failed to load at path: %s\n", cpath);
			stbi_image_free(buf);
			return texID;
		}
		else
		{
			fprintf(stdout, "texture loaded : %s , %dx%d, %d channels\n", cpath, w, h, channels);
		}

		// load into vram
		GLenum format = GL_RGBA;
		switch( channels )
		{
		case 1: format = GL_ALPHA; break;
		case 2: format = 0; break;
		case 3: format = GL_RGB; break;
		case 4:
		{
			if( !toLinear ) format = GL_RGBA;
			else format = GL_SRGB8_ALPHA8; // if hdr => 10bit
		} break;
		}

		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		// 0~1 반복 x
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_REPEAT GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// GL_NEAREST : texel만 읽음
		// GL_LINEAR : 주변점 선형보간
		// texture을 키울때는 선형보간말고 다른방법 없음.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// GL_LINEAR_MIPMAP_LINEAR : mipmap에서 찾아서 4점을 보간하고 다른 mipmap에서 찾아서 또 섞는다.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// level : 0 mipmap의 가장큰 level, 나머지는 알아서 생성
		// internalformat: 파일에 저장된값, format: 사용할값
		// 따라서 srgb에서 rgb로 선형공간으로 색이 이동되고 계산된후 다시 감마보정을 해준다.
		// GL_SRGB8_ALPHA8 8은 채널이 8비트 그냥은 10비트
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(buf);
		return texID;
	}

	class Mesh
	{
	public:
		std::string name;
		std::vector<n_model::Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture> textures;
		GLuint angles; // set size of indices
	private:
		GLuint VAO, VBO, EBO;
		GLenum drawMode;
	private:
		// disable copying
		Mesh(Mesh const&) = delete;
		Mesh& operator=(Mesh const&) = delete;
	public:
		Mesh(const std::string& _name="", const GLuint& _angle=3)
			: VAO(0), name(_name), angles(_angle)
		{
			// todo: apply every face diff draw mode
			switch( angles )
			{
			case 3: drawMode = GL_TRIANGLES; break;
			case 2: drawMode = GL_LINE_STRIP; break;
			case 4: drawMode = GL_TRIANGLE_FAN; break;
			}
		}
		Mesh(const std::vector<n_model::Vertex>& _vertices
			 , const std::vector<GLuint>& _indices
			 , const std::vector<Texture>& _textures
			 , const std::string& _name="", const GLuint& _angle=3)
			: Mesh(_name, _angle)
		{
			vertices = _vertices; // todo: fix deep copy
			indices = _indices;
			textures = _textures;
			setupMesh();
		}
		~Mesh()
		{
			clear();
		}
		void clear()
		{
			// vector은 heap 에서 언제 사라지지?
			vertices.clear();
			indices.clear();
			textures.clear();
			if( VAO!=0 )
			{
				glDeleteVertexArrays(1, &VAO);
				VAO=0;
			}
		}
		void draw(const Program& program)
		{
			// texture unifrom var name texture_specularN ...
			GLuint diffuseNr  = 0;
			GLuint specularNr = 0;
			GLuint normalNr   = 0;
			GLuint ambientNr  = 0;
			GLuint loc = 0;

			for( GLuint i=0; i<textures.size(); i++ )
			{
				std::string type = textures[i].type;
				// uniform samper2d nr is start with 1
				// omit 0th
				int backNum = 0;
				if( type=="map_Kd" )        backNum = diffuseNr++;
				else if( type=="map_Ks" )   backNum = specularNr++;
				else if( type=="map_Bump" ) backNum = normalNr++;
				else if( type=="map_Ka" )   backNum = ambientNr++;

				std::string varName = type + std::to_string(backNum);
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
				loc = glGetUniformLocation(program.ID, varName.c_str());
				glUniform1i(loc, i); // to sampler2d
			}

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // need?
			glDrawElements(drawMode, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glActiveTexture(GL_TEXTURE0);
		}
		// upload VRAM
		void setupMesh()
		{
			const size_t SIZE_OF_VERTEX = sizeof(n_model::Vertex);
			if( VAO!=0 )
				glDeleteVertexArrays(1, &VAO);
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * SIZE_OF_VERTEX, &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			// VAO setting //
			// - position
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)0);
			// - normal
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, n));
			// - tex coord
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, uv));
			// - tangent
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, tangent));
			// - bi tangnet
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, bitangent));

			// - bone ids
			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, MAX_BONE_INFLUENCE, GL_INT, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, m_BoneIDs));
			// - weights
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_model::Vertex, m_Weights));

			glBindVertexArray(0);

			// 이게 왜 가능한거지
			//glDeleteBuffers(1, &VBO);
			//glDeleteBuffers(1, &EBO);
		}
		void print() const
		{
			fprintf(stdout, "%-18s, angles %d, verts %-7lu, tris %-7lu\n"
					, name.c_str(), angles, vertices.size(), indices.size()/3);
		}
	};

	/// <summary>
	/// 모델!!
	/// </summary>
	class Model
	{
	public:
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		glm::mat4 modelMat;
		std::string name;
		std::vector<Texture> textures_loaded;
		std::vector<Mesh*> meshes;
		std::string directory; // for load texture
		GLuint verticesNum;
		GLuint trianglesNum;
		glm::vec3 boundary_max;
		glm::vec3 boundary_min;
	private:
		glm::mat4 pivotMat;
		// Disable Copying and Assignment
		Model(Model const&) = delete;
		Model& operator=(Model const&) = delete;
	public:
		Model(const std::string& _name = ""): position(glm::vec3(0)), rotation(glm::quat()), scale(glm::vec3(1))
			, pivotMat(glm::mat4(1.0f)), verticesNum(0), name(_name) // mat4(1) is identity mat
		{
			updateModelMat();
		}
		Model(const std::vector<Mesh*>& _meshes, const std::vector<Texture>& _textures, const std::string& _name="")
			:Model(_name)
		{
			meshes = _meshes;
			textures_loaded = _textures;
			updateNums();
			updateBoundary();
		}
		// load model
		// todo: string_view
		Model(const char* path): Model()
		{
			load(path);
			updateNums();
			fprintf(stdout, "model loaded : %s, vertices: %lu\n", name.c_str(), verticesNum);
			updateBoundary();
		}
		// 왜 외부에서 외부에서 생성한 mesh객체의 unique_ptr을 우측값 참조로 받아 push_back할 수 없는거지
		Model(std::function<void(std::vector<n_model::Vertex>& _vertices
			  , std::vector<GLuint>& _indices
			  , std::vector<Texture>& _textures)> genMeshFunc
			  , std::string _name ="")
			:Model()
		{
			name = _name;
			std::vector<n_model::Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<Texture> textures;
			genMeshFunc(vertices, indices, textures);

			meshes.push_back(new Mesh(vertices, indices, textures, _name));
			updateNums();
			fprintf(stdout, "\ngen model mesh : %s, vertices: %lu\n", name.c_str(), verticesNum);
			updateBoundary();
		}
		~Model()
		{
			clear();
		}
		void clear()
		{
			for( Mesh* mesh : meshes )
			{
				mesh->clear();
				delete mesh;
			}
			meshes.clear();
		}
		void resetVRAM()
		{
			for( Mesh* mesh : meshes )
			{
				mesh->setupMesh();
			}
		}
		void exportObj(const char* path)
		{
			//aiScene* scene = new aiScene();
			//aiMesh* mesh = new aiMesh();
			//mesh->mPrimitiveTypes =AI_PRIMITIVE_TYPE 

		}
		void draw(const Program& program)
		{
			program.use();
			GLuint loc = glGetUniformLocation(program.ID, "modelMat");
			glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMat));

			for( GLuint i=0; i<meshes.size(); i++ )
				meshes[i]->draw(program);
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
		}
		glm::vec3 getBoundarySize()
		{
			return boundary_max-boundary_min;
		}
		void setPivot(glm::vec3 pivot)
		{
			std::cout<<"pivot: "<<glm::to_string(pivot)<<std::endl;
			pivotMat = glm::translate(-pivot);
		}
	private:
		void load(const char* _path)
		{
			clear();
			std::string path = std::string(_path);
			std::replace(path.begin(), path.end(), '\\', '/');
			size_t slashPos = path.find_last_of('/');
			if( slashPos == std::string::npos )
			{
				name = path;
				name = name.substr(0, name.find_last_of('.'));
				directory = "";
			}
			else if( slashPos == path.length()-1 )
			{
				name = "";
				directory = path;
			}
			else
			{
				name = path.substr(slashPos+1);
				name = name.substr(0, name.find_last_of('.'));
				directory = path.substr(0, path.find_last_of('/'))+"/";
			}

			fprintf(stdout, "model loading : %s\n", name.c_str());
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
			   || !scene->mRootNode )
			{
				fprintf(stderr, "%s\n", loader.GetErrorString());
				return;
			}

			// recursive fashion
			parseNode(scene->mRootNode, scene);
		}
		void updateNums()
		{
			verticesNum = 0;
			trianglesNum = 0;
			for( Mesh* mesh : meshes )
			{
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

			for( Mesh* mesh : meshes )
			{
				for( n_model::Vertex& v : mesh->vertices )
				{
					if( boundary_max.x < v.p.x ) boundary_max.x = v.p.x;
					else if( boundary_min.x > v.p.x ) boundary_min.x = v.p.x;

					if( boundary_max.y < v.p.y ) boundary_max.y = v.p.y;
					else if( boundary_min.y > v.p.y ) boundary_min.y = v.p.y;

					if( boundary_max.z < v.p.z ) boundary_max.z = v.p.z;
					else if( boundary_min.z > v.p.z ) boundary_min.z = v.p.z;
				}
			}
			std::cout<<"boundary size: "<<glm::to_string(getBoundarySize())<<std::endl;
		}

		// load texture
		std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type)
		{
			std::vector<Texture> textures;
			for( GLuint i=0; i<mat->GetTextureCount(type); i++ )
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				const char* texPath = str.C_Str();

				// check already loaded
				// to skip loading same texture 
				bool skip = false;
				for( GLuint j=0; j<textures_loaded.size(); j++ )
				{
					if( std::strcmp(textures_loaded[j].path.data(), texPath)==0 )
					{
						textures.push_back(textures_loaded[j]);
						skip = true;
						break;
					}
				}
				// load texture
				if( !skip )
				{
					Texture texture;
					std::string fullTexPath = directory+std::string(texPath);

					texture.id = loadTextureFromFile(fullTexPath.c_str(), true);

					switch( type )
					{
					case aiTextureType_DIFFUSE: texture.type = "map_Kd"; break;
					case aiTextureType_SPECULAR: texture.type = "map_Ks"; break;
					case aiTextureType_AMBIENT: texture.type = "map_Ka"; break;
					case aiTextureType_HEIGHT: texture.type = "map_Bump"; break;
					}
					texture.path = texPath;
					textures.push_back(texture);
					textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
				}
			}
			return textures;
		}
		void parseMesh(aiMesh* mesh, const aiScene* scene)
		{
			std::vector<n_model::Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<Texture> textures;
			// now only triangle mesh
			GLuint angles = 3;

			// - per vertex
			n_model::Vertex vertex;
			for( GLuint i=0; i<mesh->mNumVertices; i++ )
			{
				vertex.p = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				if( mesh->HasNormals() )
				{
					vertex.n = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}
				// 버텍스당 최대 8개의 uv를 가질수있지만 하나만.
				if( mesh->mTextureCoords[0] )
				{
					vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
					// tangent
					vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
					// bitangent
					vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
				}
				else
				{
					vertex.uv = glm::vec2(0.0f);
				}
				vertices.push_back(vertex);
			}

			// - per triangles
			angles = mesh->mFaces[0].mNumIndices;
			for( GLuint i=0; i<mesh->mNumFaces; i++ )
			{
				aiFace face = mesh->mFaces[i];
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

			// c++
			//std::unique_ptr<Mesh>(new Mesh(vertices, indices, textures, mesh->mName.C_Str(), angles));
			// c++ 14
			meshes.push_back(new Mesh(vertices, indices, textures
							 , mesh->mName.C_Str(), angles));
		}
		void parseNode(aiNode* node, const aiScene* scene)
		{
			// in current node
			for( GLuint i=0; i<node->mNumMeshes; i++ )
			{
				parseMesh(scene->mMeshes[node->mMeshes[i]], scene);
				fprintf(stdout, "mesh loaded : ", i);
				(*meshes.back()).print();
			}
			for( GLuint i=0; i<node->mNumChildren; i++ )
			{
				parseNode(node->mChildren[i], scene);
			}
		}
	};
} // namespace lim
#endif // !MODEL_H
