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
#include <limbrary/model_view/model_io_helper.h>
#include <limbrary/texture.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/GltfMaterial.h>
#include <GLFW/glfw3.h>
#include <filesystem>

using namespace lim;
using namespace std;
using namespace glm;

namespace 
{
	Model* g_model = nullptr; // temp model
	Animator* g_animator = nullptr;
	const aiScene* g_scn;
	std::string g_model_dir;

	const int g_nr_formats = (int)aiGetImportFormatCount();
	const char* g_formats[32] = { nullptr, };


	inline vec3 toGLM( const aiVector3D& v ) {
		return { v.x, v.y, v.z };
	}
	inline quat toGLM( const aiQuaternion& q ) {
		quat tmp = { q.w, q.x, q.y, q.z };
		return (tmp);
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
			LimImportLogStream(){}
			~LimImportLogStream(){}
			virtual void write(const char* message) override
			{
				//log::pure("%s", message);
			}
	};
}



// 중복 load 막기
static Texture* loadTexture(string texPath, bool convertLinear, const char* msg)
{
	Texture* rst = nullptr;
	std::vector<Texture*>& loadedTexs = g_model->own_textures; 

	// assimp가 mtl의 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) 그래서 아래와 같이 필터링한다.
	texPath = texPath.substr(0, texPath.find_first_of(' '));
	texPath = g_model_dir + "/" + texPath;

	for( size_t i = 0; i < loadedTexs.size(); i++ ) {
		if( texPath.compare(loadedTexs[i]->file_path)==0 ) {
			rst = loadedTexs[i];
		}
	}
	if( !rst ) {
		loadedTexs.push_back(new Texture());
		lim::log::pure("%s ", msg);
		loadedTexs.back()->initFromFile(texPath, convertLinear);
		rst = loadedTexs.back();
	}
	return rst;
}



static Material* convertMaterial(aiMaterial* aiMat, bool verbose = false)
{
	Material* rst = new Material();
	Material& mat = *rst;
	aiColor3D temp3d;
	aiColor4D temp4d;
	aiString tempStr;
	float tempFloat;

	mat.name = aiMat->GetName().C_Str();

	if(verbose) log::pure("<load factors>\n");
	mat.factor_flags = Material::FF_NONE;
	if( aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, temp3d) == AI_SUCCESS ) {
		if(verbose) log::pure("base color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		mat.factor_flags |= Material::FF_COLOR_BASE;
		mat.BaseColor = toGLM(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_COLOR_SPECULAR, temp3d) == AI_SUCCESS ) {
		if(verbose) log::pure("specular color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		mat.factor_flags |= Material::FF_SPECULAR;
		mat.SpecColor = toGLM(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, tempFloat ) != AI_SUCCESS ) {
		if(verbose) log::pure("*pass wrong shininess strength: %.1f\n", tempFloat);
		//mat.specColor *= tempFloat;
	}
	if( aiMat->Get(AI_MATKEY_COLOR_AMBIENT, temp3d) == AI_SUCCESS ) {
		// log::pure("ambient color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		// mat.factor_Flags |= Material::FF_AMBIENT;
		// mat.ambientColor = toGLM(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, temp3d) == AI_SUCCESS ) {
		if(verbose) log::pure("emissive color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		mat.factor_flags |= Material::FF_EMISSION;
		mat.EmissionColor = toGLM(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_TRANSMISSION_FACTOR, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("transmission: %.1f\n", tempFloat);
		mat.factor_flags |= Material::FF_TRANSMISSION;
		mat.Transmission = tempFloat; 
	}
	if( aiMat->Get(AI_MATKEY_REFRACTI, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("refraciti: %.1f\n", tempFloat);
		mat.factor_flags |= Material::FF_REFRACITI;
		mat.Refraciti = tempFloat; 
	}
	if( aiMat->Get(AI_MATKEY_OPACITY, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("opacity: %.1f\n", tempFloat);
		mat.factor_flags |= Material::FF_OPACITY;
		mat.Opacity = tempFloat; 
	}
	if( aiMat->Get(AI_MATKEY_SHININESS, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("*pass wrong shininess: %.1f\n", tempFloat);
		//mat.shininess = tempFloat;
	}
	if( aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("roughness: %.1f\n", tempFloat);
		mat.factor_flags |= Material::FF_ROUGHNESS;
		mat.Roughness = tempFloat;
	}
	if( aiMat->Get(AI_MATKEY_METALLIC_FACTOR, tempFloat) == AI_SUCCESS ) {
		if(verbose) log::pure("metalness: %.1f\n", tempFloat);
		mat.factor_flags |= Material::FF_METALNESS;
		mat.Metalness = tempFloat; 
	}

	// normal, metalness, roughness, occlusion 은 linear space다.
	if(verbose) log::pure("<load maps>\n");
	// verbose = true;
	mat.map_Flags = Material::MF_NONE;
	if( aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_COLOR_BASE;
		mat.map_ColorBase = loadTexture(tempStr.C_Str(), true, "map_BaseColor"); // kd일때만 linear space변환
	}
	if( aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_COLOR_BASE;
		mat.map_ColorBase = loadTexture(tempStr.C_Str(), true, "map_BaseColor"); // kd일때만 linear space변환
	}
	if( aiMat->GetTexture(aiTextureType_SPECULAR, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_SPECULAR;
		mat.map_Specular = loadTexture(tempStr.C_Str(), true, "map_Specular"); // Todo: Ka Ks map 에서도 해야하나?
	}
	if( aiMat->GetTexture(aiTextureType_HEIGHT, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_HEIGHT;
		mat.map_Bump = loadTexture(tempStr.C_Str(), false, "map_Bump");
	}
	if( aiMat->GetTexture(aiTextureType_NORMALS, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_NOR;
		mat.map_Bump = loadTexture(tempStr.C_Str(), false, "map_Bump(Nor)");
	}
	if( aiMat->GetTexture(aiTextureType_AMBIENT, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_AMB_OCC;
		mat.map_AmbOcc = loadTexture(tempStr.C_Str(), false, "map_AmbOcc");
	}
	if( aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &tempStr) == AI_SUCCESS ) {
		if(verbose) log::pure("*pass map_Roughness(ROUGHNESS): %s\n", tempStr.C_Str());
		// mat.map_Flags |= Material::MF_ROUGHNESS;
		// mat.map_Roughness = loadTexture(tempStr.C_Str(), false);
	}
	if( aiMat->GetTexture(aiTextureType_METALNESS, 0, &tempStr) == AI_SUCCESS ) {
		if(verbose) log::pure("*pass map_Metalness: %s\n", tempStr.C_Str());
		// mat.map_Flags |= Material::MF_METALNESS;
		// mat.map_Metalness = loadTexture(tempStr.C_Str(), false);
	}
	if( aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_EMISSION;
		mat.map_Emission = loadTexture(tempStr.C_Str(), true, "map_Emission(Emissive)");
	}
	if( aiMat->GetTexture(aiTextureType_EMISSION_COLOR, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_EMISSION;
		mat.map_Emission = loadTexture(tempStr.C_Str(), true, "map_Emission(EmissionSolor)");
	}
	if( aiMat->GetTexture(aiTextureType_OPACITY, 0, &tempStr) == AI_SUCCESS ) {
		mat.map_Flags |= Material::MF_OPACITY;
		mat.map_Opacity = loadTexture(tempStr.C_Str(), true, "map_Opacity"); // 리니어??
	}
	if( aiMat->GetTexture(aiTextureType_SHININESS, 0, &tempStr) == AI_SUCCESS ) {
		if(verbose) log::pure("*pass map_Roughness(SHININESS): %s\n", tempStr.C_Str());
		// if(	mat.map_Flags&Material::MF_ROUGHNESS ) {
		// 	log::err("conflict map_Roughness(Shininess)\n");
		// 	std::exit(1);
		// }
		// mat.map_Flags |= Material::MF_SHININESS;
		// mat.map_Roughness = loadTexture(tempStr.C_Str(), GL_RGB8);
	}
	// Todo: assimp에서 ARM인지 RM인지 Roughness인지 구별을 못함. 일단 ARM으로 가정
	// 파일이름 분해해서 어떤 텍스쳐가 어떤순서로 있는지 찾아야함.
	// https://stackoverflow.com/questions/54116869/how-do-i-load-roughness-metallic-map-with-assimp-using-gltf-format
	if( aiMat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &tempStr) == AI_SUCCESS ) {
		if(	mat.map_Flags&(Material::MF_AMB_OCC|Material::MF_ROUGHNESS|Material::MF_METALNESS|Material::MF_SHININESS) ) {
			if(verbose) log::err("conflict ARM\n");
			std::exit(1);
		}
		mat.map_Flags |= Material::MF_ARM;
		mat.map_Roughness = loadTexture(tempStr.C_Str(), false, "map_Roughness(AMR)");
	}

	return rst;
}






static Mesh* convertMesh(const aiMesh* aiMs)
{
	Mesh* rst = new Mesh();
	Mesh& ms = *rst;
	
	ms.name = aiMs->mName.C_Str();

	{
		ms.poss.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			ms.poss[i] = toGLM( aiMs->mVertices[i] );
		}
	}

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

	if( g_animator && aiMs->HasBones() ) {
		ms.bone_infos.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			for( GLuint j=0; j<Mesh::MAX_BONE_PER_VERT; j++ ) {
				ms.bone_infos[i].idxs[j] = -1;
				ms.bone_infos[i].weights[j] = 0.f;
			}
		}
		for( int i=0; i<aiMs->mNumBones; i++ ) {
			std::string boneName = aiMs->mBones[i]->mName.C_Str();
			int boneIdx;
			if( g_animator->name_to_idx.find(boneName) == g_animator->name_to_idx.end() ) {
				boneIdx = g_animator->nr_bones;
				g_animator->nr_bones++;
				g_animator->name_to_idx[boneName] = boneIdx;
				g_animator->offsets.push_back(toGLM(aiMs->mBones[i]->mOffsetMatrix));
			} else {
				boneIdx = g_animator->name_to_idx[boneName];
			}
			auto aiWeights = aiMs->mBones[i]->mWeights;
			auto nrWeights = aiMs->mBones[i]->mNumWeights;
			
			for( int weightIdx=0; weightIdx<nrWeights; weightIdx++ ) {
				int vertIdx = aiWeights[weightIdx].mVertexId;
				float weight = aiWeights[weightIdx].mWeight;
				assert(vertIdx <= ms.bone_infos.size());
				for( int j=0; j<Mesh::MAX_BONE_PER_VERT; j++ ) {
					if( ms.bone_infos[vertIdx].idxs[j] == -1 ) {
						ms.bone_infos[vertIdx].idxs[j] = boneIdx;
						ms.bone_infos[vertIdx].weights[j] = weight;
						break;
					}
				}
			}		
		}
	} else {
		g_animator = false;
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
		g_model->nr_triangles += ms.tris.size();
	}
	if( nrNotTriFace>0 ) {
		log::err("find not tri face : nr %d\n", nrNotTriFace);
	}

	ms.initGL();

	return rst;
}

static void convertAnim(Animation& dst, const aiAnimation& src) {
	dst.name = src.mName.C_Str();
	dst.ticks_per_sec = (src.mTicksPerSecond==0.f) ? 25.f : src.mTicksPerSecond;
	dst.nr_ticks = src.mDuration;

	dst.bone_tracks.resize(src.mNumChannels);
	for( uint i=0; i<src.mNumChannels; i++ ) {
		const aiNodeAnim& aiTrack = *(src.mChannels[i]);
		Animation::BoneTrack& track = dst.bone_tracks[i];
		track.name = aiTrack.mNodeName.C_Str();
		track.target = nullptr; // after convertTree

		track.nr_poss = aiTrack.mNumPositionKeys;
		track.poss.resize(track.nr_poss);
		for( uint j=0; j<track.nr_poss; j++ ) {
			const aiVectorKey& k = aiTrack.mPositionKeys[j];
			track.poss[j] = {float(k.mTime), toGLM(k.mValue)};
		}
		track.nr_scales = aiTrack.mNumScalingKeys;
		track.scales.resize(track.nr_scales);
		for( uint j=0; j<track.nr_scales; j++ ) {
			const aiVectorKey& k = aiTrack.mScalingKeys[j];
			track.scales[j] = {float(k.mTime), toGLM(k.mValue)};
		}
		track.nr_oris = aiTrack.mNumRotationKeys;
		track.oris.resize(track.nr_oris);
		for( uint j=0; j<track.nr_oris; j++ ) {
			const aiQuatKey& k = aiTrack.mRotationKeys[j];
			track.oris[j] = {float(k.mTime), toGLM(k.mValue)};
		}
	}
}

static void convertBoneTree(BoneNode& dst, const aiNode* src) {
	dst.name = src->mName.C_Str();

	if( g_animator->name_to_idx.find(dst.name) != g_animator->name_to_idx.end() ) {
		dst.bone_idx = g_animator->name_to_idx[dst.name];
	} else {
		dst.bone_idx = -1;
	}
	for( Animation& anim : g_animator->animations ) {
		for( Animation::BoneTrack& track : anim.bone_tracks ) {
			if( track.name == dst.name ) {
				track.target = &dst;
				break;
			}
		}
	}

	dst.transform.mtx = toGLM(src->mTransformation);
	dst.transform.decomposeMtx();

	dst.childs.reserve(src->mNumChildren);
	for( size_t i=0; i< src->mNumChildren; i++ ) {
		dst.childs.push_back({});
		convertBoneTree( dst.childs.back(), src->mChildren[i]);
	}
}
static bool isBoneNode(const char* name) {
	if( g_animator->name_to_idx.find(name) != g_animator->name_to_idx.end() ) {
		return true;
	}
	for( const auto& anim : g_animator->animations ) {
		for( const auto& track : anim.bone_tracks ) {
			if( track.name == name ) {
				return true;
			}
		}
	}
	return false;
}
static void convertRdTree(RdNode& dst, const aiNode* src) 
{
	const char* name = src->mName.C_Str();
	if( isBoneNode(name) ) {
		assert( g_animator->bone_root.name=="empty" );
		convertBoneTree(g_animator->bone_root, src);
		return;
	}

	dst.name = name;
	dst.transform.mtx = toGLM(src->mTransformation);
	dst.transform.decomposeMtx();

	for( size_t i=0; i<src->mNumMeshes; i++ ) {
		const int msIdx = src->mMeshes[i];
		const aiMesh* aiMs = g_scn->mMeshes[msIdx];
		const Material* mat = g_model->own_materials[aiMs->mMaterialIndex];
		dst.meshs_mats.push_back({g_model->own_meshes[msIdx], mat});
	}

	for( size_t i=0; i< src->mNumChildren; i++ ) {
		dst.childs.push_back({});
		convertRdTree( dst.childs.back(), src->mChildren[i]);
	}
}






bool lim::Model::importFromFile(string_view modelPath, bool withAnims)
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

	clear();
	if(withAnims) {
		g_animator = &animator;
	}
	g_model = this;
	path = modelPath;
	const size_t lastSlashPos = path.find_last_of("/\\");
	const size_t dotPos = path.find_last_of('.');
	name = path.substr(lastSlashPos + 1, dotPos - lastSlashPos - 1);
	g_model_dir = (lastSlashPos == 0) ? "" : path.substr(0, lastSlashPos);


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
	pFrags |= aiProcess_LimitBoneWeights;

	aiPropertyStore* props = aiCreatePropertyStore();
	// bone node $AssimpFbx$ PreRotation Translation 추가 노드 생성
	aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);


	double elapsedTime = glfwGetTime();
	log::pure("%s model loading..\n", name.c_str());
	const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	Assimp::DefaultLogger::get()->attachStream(new LimImportLogStream(), severity);

	g_scn = aiImportFileExWithProperties(modelPath.data(), pFrags, nullptr, props);
	if( !g_scn || g_scn->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !g_scn->mRootNode ) {
		log::err("assimp_importer: %s\n\n", aiGetErrorString());
		aiReleasePropertyStore(props);
		Assimp::DefaultLogger::kill();
		return false;
	}
	aiReleasePropertyStore(props);
	Assimp::DefaultLogger::kill();
	ai_backup_flags = g_scn->mFlags;
	log::pure("done! read file with assimp in %.2fsec\n", glfwGetTime()-elapsedTime);

	/* import mats */
	elapsedTime = glfwGetTime();
	own_materials.reserve(g_scn->mNumMaterials);
	for( GLuint i=0; i<g_scn->mNumMaterials; i++ ) {
		own_materials.push_back(convertMaterial(g_scn->mMaterials[i]));
	}

	/* import my_meshes ( bone-1 ) */
	own_meshes.reserve(g_scn->mNumMeshes);
	for( GLuint i=0; i<g_scn->mNumMeshes; i++ ) {
		own_meshes.push_back(convertMesh(g_scn->mMeshes[i]));
	}
	g_animator->mtx_Bones.resize(g_animator->nr_bones);
	withAnims = (g_animator!=nullptr);

	/* import anims( bone-2 ) */
	if( withAnims ) {
		animator.animations.resize(g_scn->mNumAnimations);
		for( uint i=0; i<g_scn->mNumAnimations; i++ ) {
			convertAnim(animator.animations[i], *(g_scn->mAnimations[i]));
		}
	}

	/* set node tree structure */
	animator.bone_root.name = "empty";
	convertRdTree(root, g_scn->mRootNode);

	/* bone-3,4 */
	updateNrAndBoundary();


	log::pure("#meshs %d, #mats %d, #verts %d, #tris %d\n", own_meshes.size(), own_materials.size(), nr_vertices, nr_triangles);
	log::pure("boundary size : %f, %f, %f\n", boundary_size.x, boundary_size.y, boundary_size.z);
	log::pure("done! convert aiScene in %.2fsec\n\n", glfwGetTime()-elapsedTime);

	aiReleaseImport(g_scn);
	
	return true;
}


int lim::getNrImportFormats()
{
	return g_nr_formats;
}
const char* lim::getImportFormat(int idx)
{
	if (g_formats[0] == nullptr) {
		for (int i = 0; i < g_nr_formats; i++)
			g_formats[i] = aiGetImportFormatDescription(i)->mFileExtensions;
	}
	if (idx < 0 || idx >= g_nr_formats)
		return nullptr;
	return g_formats[idx];
}
std::string lim::findModelInDirectory(std::string_view _path)
{
	filesystem::path fpath(_path);  
	if( filesystem::is_directory(fpath) ) {
		for( const auto & entry : std::filesystem::directory_iterator(fpath) ) {
			string fm = entry.path().extension().string().c_str()+1;
			for( int i=0; i<getNrImportFormats(); i++ ) {
				std::string importFm = getImportFormat(i);
				if(importFm.find(fm)!=std::string::npos) {
					std::string rst = entry.path().string();
					return rst;
				}
			}
		}
	}
	return std::string(_path);
}