/*

2022-09-27 / im dong ye

edit from : https://github.com/assimp/assimp/issues/203

출력될때 texture의 path는 모델위치의 마지막 슬레쉬 위치로 잘려서 출력된다.


*/
#include <limbrary/model_view/model.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>
#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <filesystem>
#include <stb_image_write.h>
#include <GLFW/glfw3.h>



using namespace lim;
using namespace glm;

namespace
{
	const int _nr_formats = (int)aiGetExportFormatCount();

	const aiExportFormatDesc* _formats[32] = { nullptr, };

	Model* _src_md = nullptr;

	inline aiVector3D toAiV( const vec3& v ) {
		return { v.x, v.y, v.z };
	}
	inline aiColor3D toAiC( const vec3& c ) {
		return { c.r, c.g, c.b };
	}
	inline aiColor4D toAiC( const vec4& c ) {
		return { c.r, c.g, c.b, c.a };
	}
	inline aiMatrix4x4 toAi( const mat4& m ) {
		return aiMatrix4x4(	m[0][0], m[0][1], m[0][2], m[0][3], 
							m[1][0], m[1][1], m[1][2], m[1][3], 
							m[2][0], m[2][1], m[2][2], m[2][3], 
							m[3][0], m[3][1], m[3][2], m[3][3] );
	}

	class LimExportLogStream : public Assimp::LogStream
	{
	public:
			LimExportLogStream()
			{
			}
			~LimExportLogStream()
			{
			}
			virtual void write(const char* message) override
			{
				//log::pure("%s", message);
			}
	};

	aiMaterial* convertMaterial(Material* material)
	{
		const Material& mat = *material;
		aiMaterial* aiMat = new aiMaterial();
		aiColor3D temp3d;
		aiColor4D temp4d;
		aiString tempStr;
		float tempFloat;

		temp4d = toAiC(mat.Kd);
		aiMat->AddProperty(&temp4d, 1, AI_MATKEY_COLOR_DIFFUSE);

		tempFloat = mat.Kd.a;
		aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_OPACITY);

		temp4d = toAiC(mat.Ks);
		aiMat->AddProperty(&temp4d, 1, AI_MATKEY_COLOR_SPECULAR);

		tempFloat = mat.Ks.a;
		aiMat->AddProperty(&tempFloat, 1, AI_MATKEY_SHININESS);

		// AI_MATKEY_SHININESS_STRENGTH

		temp3d = toAiC(mat.Ka);
		aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_AMBIENT);
		
		temp3d = toAiC(mat.Ke);
		aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_EMISSIVE);

		temp3d = toAiC(mat.Tf);
		aiMat->AddProperty(&temp3d, 1, AI_MATKEY_COLOR_TRANSPARENT);


		if( mat.map_Kd != nullptr ) {
			tempStr = aiString(mat.map_Kd->path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0) );
		}
		

		if( mat.map_Ks != nullptr ) {
			tempStr = aiString(mat.map_Ks->path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0) );
		}

		if( mat.map_Ka != nullptr ) {
			tempStr = aiString(mat.map_Ka->path.data());
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0) );
		}
		
		if( mat.map_Bump != nullptr ) {
			tempStr = aiString(mat.map_Bump->path.data());
			if( mat.map_Flags & lim::Material::MF_Nor ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0) );
			}
			else if( mat.map_Flags & lim::Material::MF_Bump ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0) );
			}
			else {
				log::err("map_Bump is not nullptr but flags not defined");
			}
		}

		return aiMat;
	}

	aiMesh* convertMesh(Mesh* mesh) 
	{
		const Mesh& ms = *mesh;

		aiMesh* aiMs = new aiMesh();
		// From: https://github.com/assimp/assimp/issues/203
		aiMs->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

		GLuint idxOfMat = findIdx(_src_md->materials, ms.material);
		aiMs->mMaterialIndex = idxOfMat;

		GLuint nrTemp = mesh->poss.size();
		aiMs->mNumVertices = nrTemp;
		if( nrTemp>0 ) {
			aiMs->mVertices = new aiVector3D[nrTemp];
			for( int i=0; i<nrTemp; i++ ) {
				aiMs->mVertices[i] = toAiV(ms.poss[i]);
			}
		}

		nrTemp = mesh->nors.size();
		if( nrTemp>0 ) {
			aiMs->mNormals = new aiVector3D[nrTemp];
			for( int i=0; i<nrTemp; i++ ) {
				aiMs->mNormals[i] = toAiV(ms.nors[i]);
			}
		}

		nrTemp = mesh->uvs.size();
		if( nrTemp>0 ) {
			aiMs->mTextureCoords[0] = new aiVector3D[nrTemp];
			for( int i=0; i<nrTemp; i++ ) {
				aiMs->mTextureCoords[0][i] = {ms.uvs[i].x, ms.uvs[i].y, 0.f};
			}
		}
		
		nrTemp = mesh->tris.size();
		if( nrTemp>0 ) {
			aiMs->mNumFaces = nrTemp;
			aiMs->mFaces = new aiFace[nrTemp];
			for( int i=0; i<nrTemp; i++ ) {
				aiFace& face = aiMs->mFaces[i];
				face.mNumIndices = 3;
				face.mIndices = new unsigned int[3];
				for( int j=0; j<3; j++ ) {
					face.mIndices[j] = ms.tris[i][j];
				}
			}
		}
		
		return aiMs;
	}

	aiNode* recursiveConvertTree(const Model::Node& src)
	{
		aiNode* node = new aiNode();
		std::vector<Mesh*>& modelMeshes = _src_md->my_meshes;
		node->mTransformation = toAi(src.transformation);

		const size_t nrMeshes = src.meshes.size();
		node->mNumMeshes = nrMeshes;
		node->mMeshes = new unsigned int[nrMeshes];
		for( size_t i=0; i<nrMeshes; i++ ) {
			node->mMeshes[i] = findIdx(modelMeshes, src.meshes[i]);
		}

		const size_t nrChilds = src.childs.size();
		node->mNumChildren = nrChilds;
		node->mChildren = new aiNode*[nrChilds];
		for( size_t i=0; i< src.childs.size(); i++ ) {
			node->mChildren[i] = recursiveConvertTree(src.childs[i]);
		}
		return node;
	}

	aiScene* makeScene(Model* model)
	{
		const Model& md = *model;
		aiScene *scene = new aiScene();

		scene->mFlags = model->ai_backup_flags;

		// only one marerial
		const GLuint nrMats = md.materials.size();
		scene->mNumMaterials = nrMats;
		scene->mMaterials = new aiMaterial*[nrMats];
		for( int i = 0; i<nrMats; i++ ) {
			scene->mMaterials[i] = convertMaterial(md.materials[i]);
		}
		
		const GLuint nrMeshes = md.my_meshes.size();
		scene->mNumMeshes = nrMeshes;
		scene->mMeshes = new aiMesh*[nrMeshes];
		for( int i=0; i<nrMeshes; i++ ) {
			scene->mMeshes[i] = convertMesh(md.my_meshes[i]);
		}
		
		scene->mRootNode = recursiveConvertTree(md.root);

		return scene;
	}
};

namespace lim
{
	int getNrExportFormats()
	{
		return _nr_formats;
	}
	const aiExportFormatDesc* getExportFormatInfo(int idx)
	{
		if (_formats[0] == nullptr){
			for (int i = 0; i < _nr_formats; i++)
				_formats[i] = aiGetExportFormatDescription(i);
		}
		if (idx < 0 || idx >= _nr_formats)
			return nullptr;
		return _formats[idx];
	}

	void exportModelToFile(Model* model, size_t pIndex, std::string_view exportPath)
	{
		namespace fs = std::filesystem;
		const aiExportFormatDesc *format = _formats[pIndex];
		_src_md = model;
		std::string md_dir(exportPath);
		md_dir += "/"+_src_md->name+"_"+format->fileExtension;
		
		size_t lastSlashPos = _src_md->path.find_last_of("/\\");
		for(Texture* tex : _src_md->my_textures) {
			tex->path = tex->path.c_str() + lastSlashPos+1;
		}

		fs::path createdDir(md_dir);
		if( !std::filesystem::is_directory(createdDir) )
			fs::create_directories(createdDir);

		std::string mdPath = md_dir + "/" + model->name +'.'+format->fileExtension;
		_src_md->path = mdPath;

		/* export model */
		double elapsedTime = glfwGetTime();
		aiScene* rstScene = makeScene(model);
		log::pure("done! convert model for export in %.2f\n", glfwGetTime()-elapsedTime);
		
		elapsedTime = glfwGetTime();
		const GLuint severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
		Assimp::DefaultLogger::get()->attachStream(new LimExportLogStream(), severity);

		if( aiExportScene(rstScene, format->id, mdPath.c_str(), 0)!=AI_SUCCESS )
		{
			log::err("failed export %s\n\n", mdPath.c_str());
			delete rstScene;
			rstScene = nullptr;
			Assimp::DefaultLogger::kill();
			return;
		}
		Assimp::DefaultLogger::kill();
		delete rstScene;
		rstScene = nullptr;
		log::pure("done! export model in %.2f\n", glfwGetTime()-elapsedTime);


		/* export texture */
		elapsedTime = glfwGetTime();
		for( Texture* tex : model->my_textures )
		{
			std::string newTexPath = md_dir + "/" +  tex->path;
			tex->path = newTexPath;
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
            stbi_write_png(newTexPath.c_str(), tex->width, tex->height, 3, fileData, tex->width * 3);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &srcFbo);
			delete fileData;
			// fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
			// log::pure("copied texture: %s to %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
		}
		log::pure("done! export texture in %.2f\n\n", glfwGetTime()-elapsedTime);
	}
}