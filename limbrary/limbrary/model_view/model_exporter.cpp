/*

2022-09-27 / im dong ye

edit from : https://github.com/assimp/assimp/issues/203

출력될때 texture의 path는 모델위치의 마지막 슬레쉬 위치로 잘려서 출력된다.


*/
#include <algorithm>
#include <limbrary/model_view/model.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/general.h>
#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/GltfMaterial.h>
#include <filesystem>
#include <stb_image_write.h>
#include <GLFW/glfw3.h>
#include <stack>



using namespace lim;
using namespace glm;

namespace
{
	const int g_nr_formats = (int)aiGetExportFormatCount();

	const aiExportFormatDesc* g_formats[32] = { nullptr, };

	const Model* g_src_md = nullptr;

	std::vector<std::pair<const Mesh*, const Material*>> g_materialed_meshes;



	inline aiVector3D toAiV( const vec3& v ) {
		return { v.x, v.y, v.z };
	}
	inline aiColor3D toAiC( const vec3& c ) {
		return { c.r, c.g, c.b };
	}
	// inline aiColor4D toAiC( const vec4& c ) {
	// 	return { c.r, c.g, c.b, c.a };
	// }
	inline aiMatrix4x4 toAi( const mat4& m ) {
		return aiMatrix4x4(	m[0][0], m[0][1], m[0][2], m[0][3], 
							m[1][0], m[1][1], m[1][2], m[1][3], 
							m[2][0], m[2][1], m[2][2], m[2][3], 
							m[3][0], m[3][1], m[3][2], m[3][3] );
	}
	class LimExportLogStream : public Assimp::LogStream
	{
	public:
			LimExportLogStream(){}
			~LimExportLogStream(){}
			virtual void write(const char* message) override
			{
				//log::pure("%s", message);
			}
	};




	aiMaterial* convertMaterial(const Material& src)
	{
		aiMaterial* aiMat = new aiMaterial();
		aiColor3D temp3d;
		aiColor4D temp4d;
		aiString tempStr;
		float tempFloat;

		if(src.factor_flags & Material::FF_COLOR_BASE) {
			temp3d = toAiC(src.BaseColor);
			aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_DIFFUSE);
		}
		if(src.factor_flags & Material::FF_SPECULAR) {
			temp3d = toAiC(src.SpecColor);
			aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_SPECULAR);
		}
		// pass AI_MATKEY_SHININESS_STRENGTH
		if(src.factor_flags & Material::FF_AMBIENT) {
			// temp3d = toAiC(src.ambientColor);
			// aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_AMBIENT);
		}
		if(src.factor_flags & Material::FF_EMISSION) {
			temp3d = toAiC(src.EmissionColor);
			aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_EMISSIVE);
		}
		if(src.factor_flags & Material::FF_TRANSMISSION) {
			tempFloat = src.Transmission;
			aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_TRANSMISSION_FACTOR);
		}
		if(src.factor_flags & Material::FF_REFRACITI) {
			tempFloat = src.Refraciti;
			aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_REFRACTI);
		}
		if(src.factor_flags & Material::FF_OPACITY) {
			tempFloat = src.Opacity;
			aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_OPACITY);
		}
		// pass AI_MATKEY_SHININESS
		if(src.factor_flags & Material::FF_ROUGHNESS) {
			tempFloat = src.Opacity;
			aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_ROUGHNESS_FACTOR);
		}
		if(src.factor_flags & Material::FF_METALNESS) {
			tempFloat = src.Metalness;
			aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_METALLIC_FACTOR);
		}

		if( src.map_ColorBase ) {
			tempStr = aiString(src.map_ColorBase->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0) );
		}
		if( src.map_Specular ) {
			tempStr = aiString(src.map_Specular->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0) );
		}
		if( src.map_Bump ) {
			tempStr = aiString(src.map_Bump->file_path.data());
			if( src.map_Flags & lim::Material::MF_NOR ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0) );
			}
			else if( src.map_Flags & lim::Material::MF_HEIGHT ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0) );
			}
			else {
				log::err("map_Bump is not nullptr but flags not defined");
			}
		}
		if( src.map_AmbOcc ) {
			tempStr = aiString(src.map_AmbOcc->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0) );
		}
		if( src.map_Roughness ) {
			tempStr = aiString(src.map_Roughness->file_path.data());
			if( src.map_Flags&Material::MF_ROUGHNESS ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0) );
			}
			else if( src.map_Flags&Material::MF_SHININESS ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0) );
			}
			else if( src.map_Flags&(Material::MF_MR|Material::MF_ARM) ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_UNKNOWN, 0) );
			}
		}
		if( src.map_Metalness ) {
			tempStr = aiString(src.map_Metalness->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0) );
		}
		if( src.map_Emission ) {
			tempStr = aiString(src.map_Emission->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_EMISSION_COLOR, 0) );
		}
		if( src.map_Opacity ) {
			tempStr = aiString(src.map_Opacity->file_path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_OPACITY, 0) );
		}

		return aiMat;
	}




	aiMesh* convertMesh(const Mesh& src, const Material* mat) 
	{
		aiMesh* aiMs = new aiMesh();
		// From: https://github.com/assimp/assimp/issues/203
		aiMs->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
		aiMs->mMaterialIndex = findIdx(g_src_md->own_materials, (Material*)mat);

		GLuint size = src.poss.size();
		aiMs->mNumVertices = size;
		if( size>0 ) {
			aiMs->mVertices = new aiVector3D[size];
			for( int i=0; i<size; i++ ) {
				aiMs->mVertices[i] = toAiV(src.poss[i]);
			}
		}

		size = src.nors.size();
		if( size>0 ) {
			aiMs->mNormals = new aiVector3D[size];
			for( int i=0; i<size; i++ ) {
				aiMs->mNormals[i] = toAiV(src.nors[i]);
			}
		}

		size = src.uvs.size();
		if( size>0 ) {
			aiMs->mTextureCoords[0] = new aiVector3D[size];
			for( int i=0; i<size; i++ ) {
				aiMs->mTextureCoords[0][i] = {src.uvs[i].x, src.uvs[i].y, 0.f};
			}
		}
		
		size = src.tris.size();
		if( size>0 ) {
			aiMs->mNumFaces = size;
			aiMs->mFaces = new aiFace[size];
			for( int i=0; i<size; i++ ) {
				aiFace& face = aiMs->mFaces[i];
				face.mNumIndices = 3;
				face.mIndices = new unsigned int[3];
				for( int j=0; j<3; j++ ) {
					face.mIndices[j] = src.tris[i][j];
				}
			}
		}

		// Todo: rigging blend factors
		
		return aiMs;
	}





	void recursiveConvertTree(const RdNode& src, aiNode* dst)
	{
		dst->mName = aiString(src.name.data());
		dst->mTransformation = toAi(src.tf.mtx);

		if( src.ms!=nullptr ) {
			dst->mNumMeshes = 1;
			dst->mMeshes = new unsigned int[1];
			auto pair = std::make_pair(src.ms, src.mat);
			dst->mMeshes[0] = findIdx(g_materialed_meshes, pair);
		}

		const size_t nrChilds = src.childs.size();
		dst->mNumChildren = nrChilds;
		dst->mChildren = new aiNode*[nrChilds];
		for( size_t i=0; i< src.childs.size(); i++ ) {
			dst->mChildren[i] = new aiNode();
			recursiveConvertTree(src.childs[i], dst->mChildren[i]);
		}
	}
	

	aiScene* makeScene(const Model& md)
	{
		aiScene* scn = new aiScene();

		scn->mFlags = md.ai_backup_flags;
		scn->mNumLights = 0;

		/*
		assimp에서는 aiNode의 mesh안에 material index가 포함되어있다.
		하지만 limbrary는 mesh와 material이 독립적이고 node에서 연결시켜주기때문에
		mesh는 같지만 material이 다른경우 assimp에서는 mesh를 각각 만들어줘야한다.
		*/
		g_materialed_meshes.clear();
		md.root.dfsAll([&](const RdNode& nd) {
			if( nd.ms!=nullptr ) {
				g_materialed_meshes.push_back({nd.ms, nd.mat});
			}
			return true;
		});

		const GLuint nrMats = md.own_materials.size();
		scn->mNumMaterials = nrMats;
		scn->mMaterials = new aiMaterial*[nrMats];
		for( int i = 0; i<nrMats; i++ ) {
			scn->mMaterials[i] = convertMaterial(*md.own_materials[i]);
		}


		scn->mNumMeshes = g_materialed_meshes.size();
		scn->mMeshes = new aiMesh*[scn->mNumMeshes];
		for(int i=0; i<scn->mNumMeshes; i++) {
			auto [ms, mat] = g_materialed_meshes[i];
			scn->mMeshes[i] = convertMesh(*ms, mat);
		}


		scn->mRootNode = new aiNode();
		recursiveConvertTree(md.root, scn->mRootNode);
		g_materialed_meshes.clear();
		return scn;
	}
};



namespace lim
{
	bool Model::exportToFile(size_t pIndex, std::string_view exportDir)
	{
		namespace fs = std::filesystem;
		const aiExportFormatDesc *format = g_formats[pIndex];
		g_src_md = this;
		std::string md_dir(exportDir);
		md_dir += "/"+name+"_"+format->fileExtension;

		fs::path createdDir(md_dir);
		if( !std::filesystem::is_directory(createdDir) )
			fs::create_directories(createdDir);

		size_t lastSlashPosInOriMdPath = path.find_last_of("/\\");
		std::string mdPath = md_dir + "/" + name +'.'+format->fileExtension;
		path = mdPath;
		// export할때 모델의 상대 경로로 임시 변경
		for( auto& tex : own_textures ) {
			tex->file_path = tex->file_path.c_str() + lastSlashPosInOriMdPath+1;
		}

		/* export model */
		double elapsedTime = glfwGetTime();
		aiScene* scene = makeScene(*this);
		log::pure("done! convert model for export in %.2fsec\n", glfwGetTime()-elapsedTime);
		
		elapsedTime = glfwGetTime();
		const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		Assimp::DefaultLogger::get()->attachStream(new LimExportLogStream(), severity);

		if( aiExportScene(scene, format->id, mdPath.c_str(), 0)!=AI_SUCCESS )
		{
			log::err("failed export %s\n\n", mdPath.c_str());
			delete scene;
			scene = nullptr;
			Assimp::DefaultLogger::kill();
			return false;
		}
		Assimp::DefaultLogger::kill();
		delete scene;
		scene = nullptr;
		log::pure("done! export model in %.2fsec\n", glfwGetTime()-elapsedTime);


		/* export texture */
		elapsedTime = glfwGetTime();
		for( const auto& tex : own_textures )
		{
			std::string newTexPath = md_dir + "/" + tex->file_path.c_str();
			tex->file_path = newTexPath;

			size_t lastTexSlashPos = newTexPath.find_last_of("/\\");
			std::string newTexDirStr = newTexPath.substr(0, lastTexSlashPos);

			fs::path newTexDir(newTexDirStr);
			if( !std::filesystem::is_directory(newTexDir) )
				fs::create_directories(newTexDir);


			GLubyte* fileData = new GLubyte[3 * tex->width * tex->height ];
			GLuint srcFbo;
			glGenFramebuffers(1, &srcFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, srcFbo);
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->tex_id, 0 );

			glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0,  tex->width, tex->height, GL_RGB, GL_UNSIGNED_BYTE, fileData);

			stbi_flip_vertically_on_write(true);
			// Todo: base color texture srgb로 변환
            stbi_write_png(newTexPath.c_str(), tex->width, tex->height, 3, fileData, tex->width * 3);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &srcFbo);
			delete[] fileData;
			// fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
			// log::pure("copied texture: %s to %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
		}

		log::pure("done! export texture in %.2fsec\n\n", glfwGetTime()-elapsedTime);
		return true;
	}






	int getNrExportFormats()
	{
		return g_nr_formats;
	}
	const aiExportFormatDesc* getExportFormatInfo(int idx)
	{
		if (g_formats[0] == nullptr) {
			for (int i = 0; i < g_nr_formats; i++)
				g_formats[i] = aiGetExportFormatDescription(i);
		}
		if (idx < 0 || idx >= g_nr_formats)
			return nullptr;
		return g_formats[idx];
	}
}