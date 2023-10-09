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
#include <limbrary/model_view/model.h>
#include <limbrary/texture.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <GLFW/glfw3.h>
#include <filesystem>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	Model* _rst_md = nullptr; // temp model
	const aiScene* _scene;
	std::string _model_dir;

	const int _nr_formats = (int)aiGetImportFormatCount();
	const char* _formats[32] = { nullptr, };


	inline vec3 toGLM( const aiVector3D& v ) {
		return { v.x, v.y, v.z };
	}
	inline vec3 toGLM( const aiColor3D& c ) {
		return { c.r, c.g, c.b };
	}
	inline vec4 toGLM( const aiColor4D& c ) {
		return { c.r, c.g, c.b, c.a };
	}
	inline mat4 toGLM( const aiMatrix4x4& m ) {
		return mat4(m.a1, m.b1, m.c1, m.d1,
					m.a2, m.b2, m.c2, m.d2,
					m.a3, m.b3, m.c3, m.d3,
					m.a4, m.b4, m.c4, m.d4);
	}

	class LimImportLogStream : public Assimp::LogStream
	{
	public:
			LimImportLogStream()
			{
			}
			~LimImportLogStream()
			{
			}
			virtual void write(const char* message) override
			{
				//log::pure("%s", message);
			}
	};

	Texture* loadTexture(string texPath, GLint internalFormat=GL_RGB8)
	{
		std::vector<Texture*>& loadedTexs = _rst_md->my_textures; 

		// assimp가 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) 그래서 아래와 같이 필터링한다.
		texPath = texPath.substr(0, texPath.find_first_of(' '));
		texPath = _model_dir + "/" + texPath;

		for( size_t i = 0; i < loadedTexs.size(); i++ ) {
			if( texPath.compare(loadedTexs[i]->path)==0 ) {
				return loadedTexs[i];
			}
		}

		loadedTexs.push_back(new Texture());
		loadedTexs.back()->initFromImage(texPath, internalFormat);

		return loadedTexs.back();
	}

	Material* convertMaterial(aiMaterial* aiMat)
	{
		Material* rst = new Material();
		Material& mat = *rst;
		aiColor3D temp3d;
		aiColor4D temp4d;
		aiString tempStr;
		float tempFloat;

		mat.prog = nullptr;

		if( aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, temp4d) == AI_SUCCESS ) {
			mat.Kd = toGLM(temp4d);
		}
		if( aiMat->Get(AI_MATKEY_OPACITY, tempFloat) == AI_SUCCESS ) {
			mat.Kd.a = tempFloat; // todo
		}
		if( aiMat->Get(AI_MATKEY_COLOR_SPECULAR, temp4d) == AI_SUCCESS ) {
			mat.Ks = toGLM(temp4d);
		}
		if( aiMat->Get(AI_MATKEY_SHININESS, tempFloat) == AI_SUCCESS ) {
			mat.Ks.a = tempFloat;
		}
		if( aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, tempFloat ) != AI_SUCCESS ) {
			mat.Ks = vec4( vec3( mat.Ks )*tempFloat, mat.Ks.a );
		}
		if( aiMat->Get(AI_MATKEY_COLOR_AMBIENT, temp3d) == AI_SUCCESS ) {
			mat.Ka = toGLM(temp3d);
		}
		if( aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, temp3d) == AI_SUCCESS ) {
			mat.Ke = toGLM(temp3d);
		}
		if( aiMat->Get(AI_MATKEY_COLOR_TRANSPARENT, temp3d) == AI_SUCCESS ) {
			mat.Tf = toGLM(temp3d);
		}
		if( aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &tempStr) == AI_SUCCESS ) {
			log::pure("map_Kd ");
			mat.map_Flags |= Material::MF_Kd;
			mat.map_Kd = loadTexture(tempStr.C_Str(), GL_SRGB8); // kd일때만 linear space변환
		}
		if( aiMat->GetTexture(aiTextureType_SPECULAR, 0, &tempStr) == AI_SUCCESS ) {
			log::pure("map_Ks ");
			mat.map_Flags |= Material::MF_Ks;
			mat.map_Ks = loadTexture(tempStr.C_Str(), GL_SRGB8); // Todo: Ka Ks map 에서도 해야하나?
		}
		if( aiMat->GetTexture(aiTextureType_AMBIENT, 0, &tempStr) == AI_SUCCESS ) {
			log::pure("map_Ka ");
			mat.map_Flags |= Material::MF_Ka;
			mat.map_Ka = loadTexture(tempStr.C_Str(), GL_SRGB8);
		}
		if( aiMat->GetTexture(aiTextureType_NORMALS, 0, &tempStr) == AI_SUCCESS ) {
			log::pure("map_Nor ");
           	mat.map_Flags |= Material::MF_Nor;
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
		}
		if( aiMat->GetTexture(aiTextureType_HEIGHT, 0, &tempStr) == AI_SUCCESS ) {
			log::pure("map_Height ");
           	mat.map_Flags |= Material::MF_Height;
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
		}

		return rst;
	}

	Mesh* convertMesh(const aiMesh* aiMs)
	{
		Mesh* rst = new Mesh();
		Mesh& ms = *rst;
		vec3& boundaryMax = _rst_md->boundary_max;
		vec3& boundaryMin = _rst_md->boundary_min;
		
		ms.name = aiMs->mName.C_Str();

		ms.poss.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			ms.poss[i] = toGLM( aiMs->mVertices[i] );
			vec3& p = ms.poss[i];
			// 요소들끼리의 min max
			boundaryMax = glm::max(boundaryMax, p);
			boundaryMin = glm::min(boundaryMin, p);
		}
		_rst_md->nr_vertices += ms.poss.size();

		if( aiMs->HasNormals() ) {
			ms.nors.resize( aiMs->mNumVertices );
			for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
				ms.nors[i] = toGLM( aiMs->mNormals[i] );
			}
		}

		if( aiMs->HasTextureCoords(0) ) {
			ms.uvs.resize( aiMs->mNumVertices );
			for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
				// 2차원 uv좌표만 허용
				ms.uvs[i] = {aiMs->mTextureCoords[0][i].x, aiMs->mTextureCoords[0][i].y};
			}
		}

		if( aiMs->HasVertexColors(0) ) {
			ms.cols.resize( aiMs->mNumVertices );
			for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
				ms.cols[i] = toGLM( aiMs->mColors[0][i] );
			}
		}

		if( aiMs->HasTangentsAndBitangents() ) {
			ms.tangents.resize( aiMs->mNumVertices );
			ms.bitangents.resize( aiMs->mNumVertices );
			for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
				ms.tangents[i] = toGLM( aiMs->mTangents[i] );
				ms.bitangents[i] = toGLM( aiMs->mBitangents[i] );
			}
		}

		if( aiMs->HasBones() ) {
			//ms.bone_ids.resize( aiMs->mNumVertices );
			//ms.bending_factors.resize( aiMs->mNumVertices );
			// for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			// 	for( GLuint j=0; j<Mesh::MAX_BONE_INFLUENCE; j++ ) {
			// 		ms.bone_ids[i] =
			// 		ms.bending_factors[i] =
			// 	}
			// }
		}

		int nrNotTriFace = 0;
		if( aiMs->HasFaces() ) {
			ms.tris.reserve( aiMs->mNumFaces );
			for( GLuint i=0; i<aiMs->mNumFaces; i++ ) {
				const aiFace& face = aiMs->mFaces[i];
				if( face.mNumIndices !=3 ) {
					nrNotTriFace++;
					continue;
				}
				ms.tris.push_back( uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
			}
			_rst_md->nr_triangles += ms.tris.size();
		}
		if( nrNotTriFace>0 ) {
			log::err("find not tri face : nr %d\n", nrNotTriFace);
		}

		ms.initGL();

		return rst;
	}

	void recursiveConvertTree(const aiNode* src, Model::Node& dst) 
	{
		dst.transformation = toGLM(src->mTransformation);
		for( size_t i=0; i<src->mNumMeshes; i++ ) {
			Material* mat = nullptr;
			if( _rst_md->my_materials.size()>0 ) {
				const aiMesh* aiMs = _scene->mMeshes[src->mMeshes[i]];
				mat = _rst_md->my_materials[aiMs->mMaterialIndex];
			}
			dst.addMesh(_rst_md->my_meshes[src->mMeshes[i]], mat);
		}

		for( size_t i=0; i< src->mNumChildren; i++ ) {
			dst.childs.push_back({});
			recursiveConvertTree( src->mChildren[i], dst.childs.back());
		}
	}
}

namespace lim
{
	bool Model::importFromFile(string_view modelPath, bool unitScaleAndPivot, bool withMaterial)
	{
		const char* extension = strrchr(modelPath.data(), '.');
		if( !extension ) {
			log::err("Please provide a file with a valid extension\n");
			return false;
		}
		if ( aiIsExtensionSupported(extension) == AI_FALSE ) {
			log::err("The specified model file extension is currently\n");
			return false;
		}

		_rst_md = this;
		path = modelPath;
		const size_t lastSlashPos = path.find_last_of("/\\");
		const size_t dotPos = path.find_last_of('.');
		name = path.substr(lastSlashPos + 1, dotPos - lastSlashPos - 1);
		_model_dir = (lastSlashPos == 0) ? "" : path.substr(0, lastSlashPos);

		/* Assimp 설정 */
		GLuint pFrags = 0;
		pFrags |= aiProcess_Triangulate; // 다각형이 있다면 삼각형으로
		// pFrags |= aiProcess_GenNormals;  // 노멀이 없으면 생성
		pFrags |= aiProcess_GenSmoothNormals; // 생성하고 보간
		// pFrags |= aiProcess_FlipUVs; // opengl 텍스쳐 밑에서 읽는문제 or stbi_set_flip_vertically_on_load(true)
		// pFrags |= aiProcess_CalcTangentSpace;
		pFrags |= aiProcess_JoinIdenticalVertices; // shared vertex
		// pFrags |= aiProcess_SplitLargeMeshes : 큰 mesh를 작은 sub mesh로 나눠줌
		// pFrags |= aiProcess_OptimizeMeshes : mesh를 합쳐서 draw call을 줄인다. batching?
		pFrags |= aiProcessPreset_TargetRealtime_MaxQuality;

		double elapsedTime = glfwGetTime();
		log::pure("%s model loading..\n", name.c_str());
		const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		Assimp::DefaultLogger::get()->attachStream(new LimImportLogStream(), severity);
		_scene = aiImportFile(modelPath.data(), pFrags);
		if( !_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode ) {
			log::err("assimp_importer: %s\n\n", aiGetErrorString());
			Assimp::DefaultLogger::kill();
			return false;
		}
		Assimp::DefaultLogger::kill();
		ai_backup_flags = _scene->mFlags;
		log::pure("done! read file with assimp in %.2f\n", glfwGetTime()-elapsedTime);

		/* import mats */
		elapsedTime = glfwGetTime();
		if( withMaterial ) {
			my_materials.reserve(_scene->mNumMaterials);
			for( GLuint i=0; i<_scene->mNumMaterials; i++ ) {
				my_materials.push_back(convertMaterial(_scene->mMaterials[i]));
			}
		}

		/* import my_meshes */
		my_meshes.reserve(_scene->mNumMeshes);
		for( GLuint i=0; i<_scene->mNumMeshes; i++ ) {
			my_meshes.push_back(convertMesh(_scene->mMeshes[i]));
		}
		boundary_size = boundary_max - boundary_min;

		/* set node tree structure */
		recursiveConvertTree(_scene->mRootNode, root);
		log::pure("#meshs %d, #mats %d, #verts %d, #tris %d\n", my_meshes.size(), my_materials.size(), nr_vertices, nr_triangles);
		log::pure("boundary size : %f, %f, %f\n", name.c_str(), boundary_size.x, boundary_size.y, boundary_size.z);
		log::pure("done! convert aiScene in %.2f\n\n", glfwGetTime()-elapsedTime);

		aiReleaseImport(_scene);

		if( unitScaleAndPivot ) {
			updateUnitScaleAndPivot();
		}
		return true;
	}

	int getNrImportFormats()
	{
		return _nr_formats;
	}
	const char* getImportFormat(int idx)
	{
		if (_formats[0] == nullptr) {
			for (int i = 0; i < _nr_formats; i++)
				_formats[i] = aiGetImportFormatDescription(i)->mFileExtensions;
		}
		if (idx < 0 || idx >= _nr_formats)
			return nullptr;
		return _formats[idx];
	}
	std::string findModelInDirectory(std::string_view _path)
	{
		filesystem::path fpath(_path);  
		if( filesystem::is_directory(fpath) ) {
			for( const auto & entry : std::filesystem::directory_iterator(fpath) ) {
				string fm = entry.path().extension().string();
				for( int i=0; i<getNrImportFormats(); i++ ) {
					if(strIsSame( getImportFormat(i), fm.c_str()+1 )) {
						std::string rst = entry.path().string();
						return rst;
					}
				}
			}
		}
		return std::string(_path);
	}
}