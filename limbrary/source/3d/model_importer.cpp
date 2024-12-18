/*
	imdongye@naver.com
	fst: 2022-09-27
	lst: 2022-09-27

Todo:
	1. model path texture path 그냥 하나로 합치기
*/
#include <limbrary/3d/model.h>
#include <limbrary/3d/model_io_helper.h>
#include <limbrary/texture.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/etc.h>
#include <limbrary/tools/text.h>
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
	ModelData* g_model = nullptr; // temp model
	Animator* g_animator = nullptr;
	bool g_is_ms_has_bone = false;
	const aiScene* g_scn;
	std::string g_model_dir;

	const int g_nr_formats = (int)aiGetImportFormatCount();
	const char* g_formats[32] = { nullptr, };

	bool g_is_make_mesh_buf = true;

	inline vec3 toGlm( const aiVector3D& v ) {
		return { v.x, v.y, v.z };
	}
	inline quat toGlm( const aiQuaternion& q ) {
		quat tmp = { q.w, q.x, q.y, q.z };
		return (tmp);
	}
	inline vec3 toGlm( const aiColor3D& c ) {
		return { c.r, c.g, c.b };
	}
	inline vec4 toGlm( const aiColor4D& c ) {
		return { c.r, c.g, c.b, c.a };
	}
	inline mat4 toGlm( const aiMatrix4x4& m ) {
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
	std::vector<OwnPtr<Texture>>& loadedTexs = g_model->own_textures; 

	// assimp가 mtl의 뒤에오는 옵션을 읽지 않음 (ex: eye.png -bm 0.4) 그래서 아래와 같이 필터링한다.
	texPath = texPath.substr(0, texPath.find_first_of(' '));
	texPath = g_model_dir + texPath;

	for( size_t i = 0; i < loadedTexs.size(); i++ ) {
		if( texPath.compare(loadedTexs[i]->file_path)==0 ) {
			rst = loadedTexs[i].raw;
		}
	}
	if( !rst ) {
		rst = new Texture();
		loadedTexs.push_back(rst);
		lim::log::pure("%s ", msg);
		const aiTexture* aTex = g_scn->GetEmbeddedTexture(texPath.c_str());
		if( aTex ) {
			// From: https://github.com/assimp/assimp/issues/408
			log::pure("embbed tex fm hint: %s\n", aTex->achFormatHint);
			rst->initFromMem((unsigned char*)aTex->pcData, {aTex->mWidth, aTex->mHeight}
				, convertLinear, aTex->mFilename.C_Str());
		}
		else {
			rst->initFromFile(texPath.c_str(), convertLinear);
		}
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
		mat.BaseColor = toGlm(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_COLOR_SPECULAR, temp3d) == AI_SUCCESS ) {
		if(verbose) log::pure("specular color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		mat.factor_flags |= Material::FF_SPECULAR;
		mat.SpecColor = toGlm(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, tempFloat ) != AI_SUCCESS ) {
		if(verbose) log::pure("*pass wrong shininess strength: %.1f\n", tempFloat);
		//mat.specColor *= tempFloat;
	}
	if( aiMat->Get(AI_MATKEY_COLOR_AMBIENT, temp3d) == AI_SUCCESS ) {
		// log::pure("ambient color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		// mat.factor_Flags |= Material::FF_AMBIENT;
		// mat.ambientColor = toGlm(temp3d); 
	}
	if( aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, temp3d) == AI_SUCCESS ) {
		if(verbose) log::pure("emissive color: %.1f %.1f %.1f\n", temp3d.r, temp3d.g, temp3d.b);
		mat.factor_flags |= Material::FF_EMISSION;
		mat.EmissionColor = toGlm(temp3d); 
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
			ms.poss[i] = toGlm( aiMs->mVertices[i] );
		}
	}

	if( aiMs->HasNormals() ) {
		ms.nors.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			ms.nors[i] = toGlm( aiMs->mNormals[i] );
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
			ms.cols[i] = toGlm( aiMs->mColors[0][i] );
		}
	}

	if( aiMs->HasTangentsAndBitangents() ) {
		ms.tangents.resize( aiMs->mNumVertices );
		ms.bitangents.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			ms.tangents[i] = toGlm( aiMs->mTangents[i] );
			ms.bitangents[i] = toGlm( aiMs->mBitangents[i] );
		}
	}

	if( g_animator && aiMs->HasBones() ) {
		g_is_ms_has_bone = true;
		ms.bone_infos.resize( aiMs->mNumVertices );
		for( GLuint i=0; i<aiMs->mNumVertices; i++ ) {
			for( GLuint j=0; j<Mesh::MAX_BONE_PER_VERT; j++ ) {
				ms.bone_infos[i].idxs[j] = -1;
				ms.bone_infos[i].weights[j] = 0.f;
			}
		}
		for( int i=0; i<aiMs->mNumBones; i++ ) {
			const aiBone* aiBone = aiMs->mBones[i];
			std::string boneName = aiBone->mName.C_Str();
			int boneIdx;
			if( !isIn(g_model->name_to_weighted_bone_idx, boneName) ) {
				boneIdx = g_model->nr_weighted_bones;
				g_model->nr_weighted_bones++;
				g_model->name_to_weighted_bone_idx[boneName] = boneIdx;
				g_model->weighted_bone_offsets.push_back(toGlm(aiBone->mOffsetMatrix));
			} else {
				boneIdx = g_model->name_to_weighted_bone_idx[boneName];
			}
			auto aiWeights = aiBone->mWeights;
			auto nrWeights = aiBone->mNumWeights;
			
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
	}

	int nrNotTriFace = 0;
	if( aiMs->HasFaces() ) {
		ms.tris.reserve( aiMs->mNumFaces );
		for( GLuint i=0; i<aiMs->mNumFaces; i++ ) {
			const aiFace& face = aiMs->mFaces[i];
			if( face.mNumIndices == 3 ) {
				ms.tris.push_back( uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
			} else if( face.mNumIndices == 4 ) {
				assert(false); // already aiProcess_Triangulate
				ms.tris.push_back( uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[3]));
				ms.tris.push_back( uvec3(face.mIndices[3], face.mIndices[2], face.mIndices[1]));
			} else {
				assert(false);
				nrNotTriFace++;
			}
		}
		g_model->total_tris += ms.tris.size();
	}
	if( nrNotTriFace>0 ) {
		log::err("find not tri face : nr %d\n", nrNotTriFace);
	}

	if( g_is_make_mesh_buf ) {
		ms.initGL();
	}

	return rst;
}

static void convertAnim(Animation& dst, const aiAnimation& src) {
	dst.name = src.mName.C_Str();
	dst.ticks_per_sec = (src.mTicksPerSecond==0.f) ? 25.f : src.mTicksPerSecond;
	dst.nr_ticks = src.mDuration;
	dst.nr_tracks = src.mNumChannels;

	dst.tracks.resize(dst.nr_tracks);
	for( uint i=0; i<dst.nr_tracks; i++ ) {
		const aiNodeAnim& srcTrack = *(src.mChannels[i]);
		Animation::Track& track = dst.tracks[i];
		track.name = srcTrack.mNodeName.C_Str();
		track.idx_bone_node = -1; // after convertTree

		track.nr_poss = srcTrack.mNumPositionKeys;
		track.poss.reserve(track.nr_poss);
		for( uint j=0; j<track.nr_poss; j++ ) {
			const aiVectorKey& k = srcTrack.mPositionKeys[j];
			track.poss.push_back({(float)k.mTime, toGlm(k.mValue)});
		}
		track.nr_scales = srcTrack.mNumScalingKeys;
		track.scales.reserve(track.nr_scales);
		for( uint j=0; j<track.nr_scales; j++ ) {
			const aiVectorKey& k = srcTrack.mScalingKeys[j];
			track.scales.push_back({(float)k.mTime, toGlm(k.mValue)});
		}
		track.nr_oris = srcTrack.mNumRotationKeys;
		track.oris.reserve(track.nr_oris);
		for( uint j=0; j<track.nr_oris; j++ ) {
			const aiQuatKey& k = srcTrack.mRotationKeys[j];
			track.oris.push_back({(float)k.mTime, toGlm(k.mValue)});
		}
	}
}
// static bool isEndPostfix(std::string name) {
// 	if( name.length() < 4 )
// 		return false;
// 	name = name.substr(name.size()-4, 4);
// 	return name=="_end";
// }
static void addBoneNode(int idxParent, const mat4 mtxParent, const aiNode* src) {
	assert(src->mNumMeshes==0);
	g_animator->nr_bones++;
	g_animator->bones.emplace_back();
	int curIdx = g_animator->nr_bones-1;
	BoneNode& dst = g_animator->bones.back();
	dst.name = src->mName.C_Str();
	dst.nr_childs = src->mNumChildren;

	if( isIn(g_model->name_to_weighted_bone_idx, dst.name) ) {
		dst.idx_weighted_bone = g_model->name_to_weighted_bone_idx[dst.name];
	} else {
		dst.idx_weighted_bone = -1;
	}
	dst.idx_parent_bone_node = idxParent;

	for( Animation& anim : g_model->animations ) {
		for( Animation::Track& track : anim.tracks ) {
			if( track.name == dst.name ) {
				track.idx_bone_node = curIdx;
				break;
			}
		}
	}

	dst.tf.mtx = toGlm(src->mTransformation);
	dst.tf.decomposeMtx();
	dst.mtx_model_space = mtxParent * dst.tf.mtx;

	
	for( size_t i=0; i< src->mNumChildren; i++ ) {
		aiNode* aiChild = src->mChildren[i];
		// if(isEndPostfix(aiChild->mName.C_Str())) {
		// 	continue;
		// }
		
		// !! do not use dst pointer 
		// 	  because vector array memory can be moved by under recursive func
		addBoneNode( curIdx, g_animator->bones[curIdx].mtx_model_space, aiChild );
	}
}
static bool isBoneNode(aiNode* aiChild) {
	if( aiChild->mNumMeshes > 0 ) {
		return false;
	}
	string name = aiChild->mName.C_Str();
	if( name == "Armature" ) {
		if( aiChild->mNumChildren > 1 ) {
			return false;
		}
		return true;
	}
	if( isIn(g_model->name_to_weighted_bone_idx, name ) ) {
		return true;
	}
	for( const auto& anim : g_model->animations ) {
		for( const auto& track : anim.tracks ) {
			if( track.name == name ) {
				return true;
			}
		}
	}
	return false;
}
// assume
static void convertRdTree(RdNode& dstParent, const aiNode* src, int depth) 
{
	Transform tf;
	tf.mtx = toGlm(src->mTransformation);
	tf.decomposeMtx();
	// todo determine identity matrix

	for( size_t i=0; i<src->mNumMeshes; i++ ) {
		const int msIdx = src->mMeshes[i];
		const aiMesh* aiMs = g_scn->mMeshes[msIdx];
		RdNode& rdNd = dstParent.addChild(src->mName.C_Str()
			, g_model->own_meshes[msIdx].raw
			, g_model->own_materials[aiMs->mMaterialIndex].raw);
		rdNd.tf = tf;
	}

	// transform node( parent )
	if( src->mNumChildren > 0 ) {
		RdNode& tfNd = dstParent.addChild(src->mName.C_Str(), nullptr, nullptr);
		tfNd.tf = tf;

		tfNd.childs.reserve(src->mNumChildren);
		for( size_t i=0; i< src->mNumChildren; i++ ) {
			aiNode* aiChild = src->mChildren[i];
			if( g_is_ms_has_bone && isBoneNode(aiChild) ) {
				// assume that model has only one bone tree
				assert(g_animator->nr_bones==0);
				assert(g_model->depth_of_bone_root_in_rdtree<1); // normaly bone root in first node

				g_model->depth_of_bone_root_in_rdtree = depth;
				addBoneNode( -1, mat4(1), aiChild );
				continue;
			}
			convertRdTree( tfNd, aiChild, depth+1 );
		}
	}
}






bool lim::ModelData::importFromFile(
	const char* modelPath, bool withAnims
	, bool scaleAndPivot, float maxSize, vec3 pivot
	, GLuint aiPostProcessFlags, GLuint aiPostProcessFlags2
	, bool makeMeshBuf
)
{
	const char* extension = getExtension(modelPath);
	if( !extension ) {
		log::err("Please provide a file with a valid extension\n");
		return false;
	}
	if ( aiIsExtensionSupported(extension) == AI_FALSE ) {
		log::err("The specified model file extension is currently\n");
		return false;
	}

	clear();

	g_model = this;
	path = modelPath;
	name = getFileNameWithoutExt(path.c_str());
	g_model_dir = getDirectory(path);

	g_is_make_mesh_buf = makeMeshBuf;

	// bone node $AssimpFbx$ PreRotation Translation 추가 노드 생성
	aiPropertyStore* props = aiCreatePropertyStore();
	aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);


	double elapsedTime = glfwGetTime();
	log::pure("%s model loading..\n", name.c_str());
	const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	Assimp::DefaultLogger::get()->attachStream(new LimImportLogStream(), severity);

	g_scn = aiImportFileExWithProperties(modelPath, aiPostProcessFlags, nullptr, props);
	if( !g_scn || g_scn->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !g_scn->mRootNode ) {
		log::err("assimp_importer: %s\n\n", aiGetErrorString());
		aiReleasePropertyStore(props);
		Assimp::DefaultLogger::kill();
		return false;
	}
	// tangent generation need 2pass 
	// From: https://stackoverflow.com/questions/17796445/is-it-possible-to-use-assimp-to-generate-tangents-if-you-also-use-it-to-generate
	if( aiPostProcessFlags2 != 0 ) {
		const aiScene* rstPass2 = aiApplyPostProcessing(g_scn, aiPostProcessFlags2);
		if( rstPass2 ) {
			log::pure("done pass2 postprocessing\n");

			aiReleaseImport(g_scn);
			g_scn = rstPass2;
		}
		else {
			log::err("pass2 postprocessing is failed: %s\n\n", aiGetErrorString());
		}
	} else {
		log::pure("pass2 postprocessing is skipped\n");
	}

	aiReleasePropertyStore(props);
	Assimp::DefaultLogger::kill();
	ai_backup_flags = g_scn->mFlags;
	log::pure("read file with assimp in %.2fsec\n", glfwGetTime()-elapsedTime);

	/* import mats */
	elapsedTime = glfwGetTime();
	own_materials.reserve(g_scn->mNumMaterials);
	for( GLuint i=0; i<g_scn->mNumMaterials; i++ ) {
		own_materials.push_back(convertMaterial(g_scn->mMaterials[i]));
	}

	/* import my_meshes, make bone_name_map ( bone-1 ) */
	g_is_ms_has_bone = false;
	g_animator = nullptr;
	if( withAnims ) {
		own_animator = new Animator();
		g_animator = own_animator.raw;
	}
	own_meshes.reserve(g_scn->mNumMeshes);
	for( GLuint i=0; i<g_scn->mNumMeshes; i++ ) {
		own_meshes.push_back(convertMesh(g_scn->mMeshes[i]));
	}

	if( g_is_ms_has_bone ) {
		/* import anims( bone-2 ) */
		own_animator->init(this);
		animations.resize(g_scn->mNumAnimations);
		for( uint i=0; i<g_scn->mNumAnimations; i++ ) {
			convertAnim(animations[i], *(g_scn->mAnimations[i]));
		}
		own_animator->setAnim(&animations[0]);
	
		/* set bone node tree structure (bone-3)
		and link animation (bone-4) 
		assume that first node is not bone root node
		*/
		own_animator->nr_bones = 0;
		own_animator->bones.clear();
		own_animator->bones.reserve(nr_weighted_bones+10);
		g_model->depth_of_bone_root_in_rdtree = -1;
		assert( isBoneNode(g_scn->mRootNode)==false );
	}
	root.name = "rootStart";
	convertRdTree(root, g_scn->mRootNode, 0);

	if( g_is_ms_has_bone ) {
		assert(own_animator->nr_bones>0&&g_model->depth_of_bone_root_in_rdtree>=0);

		/* update bone offset with bind pose (bone-5) */
		/* sometimes defualt offset mtx is wrong */
		// mat4 invGlobalTf = inverse(global_transform);
		// for( mat4& offset : weighted_bone_offsets ) {
		// 	offset = offset*global_transform;
		// }

		// for( const BoneNode& nd : animator.bones ) {
		// 	int boneIdx = nd.idx_weighted_bone;
		// 	if( boneIdx<0 )
		// 		continue;
		// 	weighted_bone_offsets[boneIdx] = glm::inverse(nd.mtx_model_space);
		// }
		own_animator->updateMtxBones();
	}



	

	// !Important! : rid root transform
	if( scaleAndPivot ) {
		root.tf = Transform();
	}


	// update import info	
	total_verts = 0;
	total_tris = 0;
	boundary_max = glim::minimum_vec3;
	boundary_min = glim::maximum_vec3;

	root.updateGlobalTransform();

	root.dfsAll([&](const RdNode& nd) {
		if( nd.ms==nullptr ) {
			return true;
		}
		total_verts += nd.ms->poss.size();
		total_tris += nd.ms->tris.size();
		for(glm::vec3 p : nd.ms->poss) {
			p = nd.mtx_global * glm::vec4(p, 1);
			boundary_max = glm::max(boundary_max, p);
			boundary_min = glm::min(boundary_min, p);
		}
		return true;
	});
	boundary_size = boundary_max-boundary_min;



	/* scale and pivot */
	if( scaleAndPivot ) {
		depth_of_bone_root_in_rdtree++;

		float max_axis_length = glm::max(glm::max(boundary_size.x, boundary_size.y), boundary_size.z);
		// float min_axis_length = glm::min(glm::min(boundary_size.x, boundary_size.y), boundary_size.z);
		glm::vec3 normScale = glm::vec3(maxSize/max_axis_length);

		vec3 halfBoundaryBasis = boundary_size*0.5f*normScale;
		vec3 normPos =  -boundary_min*normScale - halfBoundaryBasis - halfBoundaryBasis*pivot;

		RdNode pivotTemp = root;
		pivotTemp.parent = &root;

		root.clear();
		root.childs.emplace_back(move(pivotTemp));
		RdNode& pivotNode = root.childs.back();
		pivotNode.name = "pivot";
		pivotNode.tf.pos = normPos;
		pivotNode.tf.scale = normScale;
		pivotNode.tf.update();

		root.updateGlobalTransform();
	}


	log::pure("#meshs %d, #mats %d, #verts %d, #tris %d\n", own_meshes.size(), own_materials.size(), total_verts, total_tris);
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
std::string lim::findModelInDirectory(const char* _path)
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