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
#include <GLFW/glfw3.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	Model* rst = nullptr; // temp model
	string md_dir;


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

	Texture* loadTexture(string texPath, GLint internalFormat=GL_RGB8)
	{
		vector<Texture*>& loadedTestures = rst->textures_loaded;

		// assimp가 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) 그래서 아래와 같이 필터링한다.
		texPath = texPath.substr(0, texPath.find_first_of(' '));
		texPath = md_dir + texPath;

		for( size_t i = 0; i < loadedTestures.size(); i++ ) {
			if( texPath.compare(loadedTestures[i]->path)==0 ) {
				return loadedTestures[i];
			}
		}

		// kd일때만 linear space변환
		Texture* tex = new Texture();
		tex->initFromImageAuto(texPath);
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
			mat.map_Kd = loadTexture(tempStr.C_Str(), GL_SRGB8);
			mat.map_Kd->tag = "map_Kd";
		}
		if( aiMat->GetTexture(aiTextureType_SPECULAR, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Ks = loadTexture(tempStr.C_Str(), GL_SRGB8);
			mat.map_Ks->tag = "map_Ks";
		}
		if( aiMat->GetTexture(aiTextureType_AMBIENT, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Ka = loadTexture(tempStr.C_Str(), GL_SRGB8);
			mat.map_Ka->tag = "map_Ka";
		}
		if( aiMat->GetTexture(aiTextureType_NORMALS, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
			mat.bumpIsNormal = true;
			mat.map_Bump->tag = "map_Bump";
		}
		if( aiMat->GetTexture(aiTextureType_HEIGHT, 0, &tempStr) == AI_SUCCESS ) {
			mat.map_Bump = loadTexture(tempStr.C_Str(), GL_RGB8);
			mat.map_Bump->tag = "map_Bump";
			mat.bumpIsNormal = false;
		}

		return pMat;
	}

	Mesh* convertMesh(aiMesh* aiMesh)
	{
		Mesh* pMs = new Mesh();
		Mesh& mesh = *pMs;
		vec3& boundaryMax = rst->boundary_max;
		vec3& boundaryMin = rst->boundary_min;

		const vector<Material*>& mats = rst->materials;

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
		rst->nr_vertices += mesh.poss.size();
		
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
			rst->nr_triangles += mesh.tris.size();
		}
		if( nrNotTriFace>0 ) {
			log::err("find not tri face # %d\n", nrNotTriFace);
		}

		mesh.initGL();

		return pMs;
	}

	void recursiveConvertTree(const Model& md, Model::Node& to, const aiNode* from) 
	{
		to.transformation = toGLM(from->mTransformation);
		for( size_t i=0; i<from->mNumMeshes; i++ )
			to.meshes.push_back( md.meshes[from->mMeshes[i]] );
		for( size_t i=0; i< from->mNumChildren; i++ ) {
			to.childs.push_back({});
			recursiveConvertTree(md, to.childs.back(), from->mChildren[i]);
		}
	}
}

namespace lim
{
	Model *importModelFromFile(string_view modelPath, bool normalizeAndPivot, bool withMaterial)
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

		rst = new Model();
		rst->path = modelPath;
		string& spath = rst->path;
		const size_t lastSlashPos = spath.find_last_of("/\\");
		const size_t dotPos = spath.find_last_of('.');
		rst->name = spath.substr(lastSlashPos + 1, dotPos - lastSlashPos - 1);
		md_dir = (lastSlashPos == 0) ? "" : spath.substr(0, lastSlashPos) + "/";
		
		log::pure("model loading : %s\n", rst->name.c_str());

		double elapsedTime = glfwGetTime();

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
		const aiScene* scene = aiImportFile(modelPath.data(), pFrags);

		log::pure("[%s] read assimp in %.2lf\n", rst->name.c_str(), glfwGetTime()-elapsedTime);


		if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) {
			log::err("[import assimp] %s\n", aiGetErrorString());
			delete rst;
			return nullptr;
		}

		elapsedTime = glfwGetTime();

		/* import mats */
		if( withMaterial ) {
			rst->materials.resize(scene->mNumMaterials);
			for( GLuint i=0; i<scene->mNumMaterials; i++ ) {
				rst->materials[i] = convertMaterial(scene->mMaterials[i]);
			}
		}


		/* import meshes */
		rst->meshes.resize(scene->mNumMeshes);
		for( GLuint i=0; i<scene->mNumMeshes; i++ ) {
			rst->meshes[i] = convertMesh(scene->mMeshes[i]);
		}

		/* set node tree structure */
		recursiveConvertTree(*rst, rst->root, scene->mRootNode);
		log::pure("[%s] convert lim::Model in %.2lf\n", rst->name.c_str(), glfwGetTime()-elapsedTime);
		log::pure("[%s] #meshs %d, #mats %d, #verts %d, #tris %d\n", rst->name.c_str(), rst->meshes.size(), rst->materials.size(), rst->nr_vertices, rst->nr_triangles);


		aiReleaseImport(scene);

		if( normalizeAndPivot ) {
			rst->updateUnitScaleAndPivot();
			rst->updateModelMat();
		}
		return rst;
	}
}