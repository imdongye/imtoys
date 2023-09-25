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
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <GLFW/glfw3.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	Model* _rst = nullptr; // temp model
	std::string _model_dir;


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
		vector<Texture*>& loadedTestures = _rst->my_textures;

		// assimp가 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) 그래서 아래와 같이 필터링한다.
		texPath = texPath.substr(0, texPath.find_first_of(' '));
		texPath = _model_dir + "/" + texPath;

		for( size_t i = 0; i < loadedTestures.size(); i++ ) {
			if( texPath.compare(loadedTestures[i]->path)==0 ) {
				return loadedTestures[i];
			}
		}

		// kd일때만 linear space변환
		Texture* tex = new Texture();
		tex->initFromImage(texPath, internalFormat);
		loadedTestures.push_back(tex);

		return tex;
	}

	Material* convertMaterial(aiMaterial* aiMat)
	{
		Material* pMat = new Material();
		Material& mat = *pMat;
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
			mat.map_Flags |= Material::MF_Kd;
			mat.map_Kd = loadTexture(tempStr.C_Str(), GL_SRGB8);
		}
		if( aiMat->GetTexture(aiTextureType_SPECULAR, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Flags |= Material::MF_Ks;
			mat.map_Ks = loadTexture(tempStr.C_Str(), GL_SRGB8);
		}
		if( aiMat->GetTexture(aiTextureType_AMBIENT, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Flags |= Material::MF_Ka;
			mat.map_Ka = loadTexture(tempStr.C_Str(), GL_SRGB8);
		}
		if( aiMat->GetTexture(aiTextureType_NORMALS, 0, &tempStr) == AI_SUCCESS ) {
           	mat.map_Flags |= Material::MF_Nor;
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
		}
		if( aiMat->GetTexture(aiTextureType_HEIGHT, 0, &tempStr) == AI_SUCCESS ) {
           	mat.map_Flags |= Material::MF_Bump;
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
		}

		return pMat;
	}

	Mesh* convertMesh(aiMesh* aiMesh)
	{
		Mesh* pMs = new Mesh();
		Mesh& mesh = *pMs;
		vec3& boundaryMax = _rst->boundary_max;
		vec3& boundaryMin = _rst->boundary_min;

		const vector<Material*>& mats = _rst->materials;

		if( mats.size()>0 ) {
			mesh.material = mats[aiMesh->mMaterialIndex];
		} else {
			mesh.material = nullptr;
		}

		mesh.poss.resize( aiMesh->mNumVertices );
		for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
			mesh.poss[i] = toGLM( aiMesh->mVertices[i] );
			vec3& p = mesh.poss[i];
			// 요소들끼리의 min max
			boundaryMax = glm::max(boundaryMax, p);
			boundaryMin = glm::min(boundaryMin, p);
		}
		_rst->nr_vertices += mesh.poss.size();

		if( aiMesh->HasNormals() ) {
			mesh.nors.resize( aiMesh->mNumVertices );
			for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
				mesh.nors[i] = toGLM( aiMesh->mNormals[i] );
			}
		}

		if( aiMesh->HasTextureCoords(0) ) {
			mesh.uvs.resize( aiMesh->mNumVertices );
			for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
				// Todo: why aiVector3D
				mesh.uvs[i] = {aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y};
			}
		}

		if( aiMesh->HasVertexColors(0) ) {
			mesh.cols.resize( aiMesh->mNumVertices );
			for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
				mesh.cols[i] = toGLM( aiMesh->mColors[0][i] );
			}
		}

		if( aiMesh->HasTangentsAndBitangents() ) {
			mesh.tangents.resize( aiMesh->mNumVertices );
			mesh.bitangents.resize( aiMesh->mNumVertices );
			for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
				mesh.tangents[i] = toGLM( aiMesh->mTangents[i] );
				mesh.bitangents[i] = toGLM( aiMesh->mBitangents[i] );
			}
		}

		if( aiMesh->HasBones() ) {
			//mesh.bone_ids.resize( aiMesh->mNumVertices );
			//mesh.bending_factors.resize( aiMesh->mNumVertices );
			// for( GLuint i=0; i<aiMesh->mNumVertices; i++ ) {
			// 	for( GLuint j=0; j<Mesh::MAX_BONE_INFLUENCE; j++ ) {
			// 		mesh.bone_ids[i] =
			// 		mesh.bending_factors[i] =
			// 	}
			// }
		}

		int nrNotTriFace = 0;
		if( aiMesh->HasFaces() ) {
			mesh.tris.reserve( aiMesh->mNumFaces );
			for( GLuint i=0; i<aiMesh->mNumFaces; i++ ) {
				const aiFace& face = aiMesh->mFaces[i];
				if( face.mNumIndices !=3 ) {
					nrNotTriFace++;
					continue;
				}
				mesh.tris.push_back( uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
			}
			_rst->nr_triangles += mesh.tris.size();
		}
		if( nrNotTriFace>0 ) {
			log::err("find not tri face : nr %d\n", nrNotTriFace);
		}

		mesh.initGL();

		return pMs;
	}

	void recursiveConvertTree(const Model& md, const aiNode* src, Model::Node& dst) 
	{
		dst.transformation = toGLM(src->mTransformation);
		for( size_t i=0; i<src->mNumMeshes; i++ )
			dst.meshes.push_back( md.my_meshes[src->mMeshes[i]] );
		for( size_t i=0; i< src->mNumChildren; i++ ) {
			dst.childs.push_back({});
			recursiveConvertTree(md, src->mChildren[i], dst.childs.back());
		}
	}
}

namespace lim
{
	Model* importModelFromFile(string_view modelPath, bool unitScaleAndPivot, bool withMaterial)
	{
		const char* extension = strrchr(modelPath.data(), '.');
		if( !extension ) {
			log::err("Please provide a file with a valid extension\n");
			return nullptr;
		}
		if ( aiIsExtensionSupported(extension) == AI_FALSE ) {
			log::err("The specified model file extension is currently\n");
			return nullptr;
		}

		_rst = new Model();
		_rst->path = modelPath;
		string& path = _rst->path;
		const size_t lastSlashPos = path.find_last_of("/\\");
		const size_t dotPos = path.find_last_of('.');
		_rst->name = path.substr(lastSlashPos + 1, dotPos - lastSlashPos - 1);
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
		const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
		Assimp::DefaultLogger::get()->attachStream(new LimImportLogStream(), severity);
		const aiScene* scene = aiImportFile(modelPath.data(), pFrags);
		if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) {
			log::err("[import assimp] %s\n\n", aiGetErrorString());
			delete _rst;
			Assimp::DefaultLogger::kill();
			return nullptr;
		}
		Assimp::DefaultLogger::kill();
		_rst->ai_backup_flags = scene->mFlags;
		log::pure("done! read file with assimp in %.2f\n", glfwGetTime()-elapsedTime);

		/* import mats */
		elapsedTime = glfwGetTime();
		if( withMaterial ) {
			_rst->materials.resize(scene->mNumMaterials);
			for( GLuint i=0; i<scene->mNumMaterials; i++ ) {
				_rst->materials[i] = convertMaterial(scene->mMaterials[i]);
			}
		}

		/* import my_meshes */
		_rst->my_meshes.resize(scene->mNumMeshes);
		for( GLuint i=0; i<scene->mNumMeshes; i++ ) {
			_rst->my_meshes[i] = convertMesh(scene->mMeshes[i]);
		}
		_rst->boundary_size = _rst->boundary_max - _rst->boundary_min;

		/* set node tree structure */
		recursiveConvertTree(*_rst, scene->mRootNode, _rst->root);
		log::pure("[%s] convert lim::Model in %.2lf\n", _rst->name.c_str(), glfwGetTime()-elapsedTime);
		log::pure("[%s] #meshs %d, #mats %d, #verts %d, #tris %d\n", _rst->name.c_str(), _rst->my_meshes.size(), _rst->materials.size(), _rst->nr_vertices, _rst->nr_triangles);
		log::pure("[%s] boundary size : %f, %f, %f\n", _rst->name.c_str(), _rst->boundary_size.x, _rst->boundary_size.y, _rst->boundary_size.z);
		log::pure("done! read file with assimp in %.2f\n\n", glfwGetTime()-elapsedTime);

		aiReleaseImport(scene);

		if( unitScaleAndPivot ) {
			_rst->updateUnitScaleAndPivot();
		}
		return _rst;
	}
}