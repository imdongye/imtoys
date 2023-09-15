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
#include <filesystem>


using namespace lim;
using namespace glm;

namespace
{
	const int nr_formats = (int)aiGetExportFormatCount();

	const aiExportFormatDesc *formats[32] = { nullptr, };

	std::string md_dir;

	Model* src_md = nullptr;
	aiScene* rst_scene = nullptr;

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


		const GLuint slashPos = md_dir.size(); // todo: test

		if( mat.map_Kd != nullptr ) {
			tempStr = aiString(mat.map_Kd->path.data()+slashPos);
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0) );
		}
		

		if( mat.map_Ks != nullptr ) {
			tempStr = aiString(mat.map_Ks->path.data()+slashPos);
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0) );
		}

		if( mat.map_Ka != nullptr ) {
			tempStr = aiString(mat.map_Ka->path.data()+slashPos);
			aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0) );
		}
		
		if( mat.map_Bump != nullptr ) {
			tempStr = aiString(mat.map_Bump->path.data()+slashPos);
			if( mat.bumpIsNormal ) {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0) );
			}
			else {
				aiMat->AddProperty( &tempStr, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0) );
			}
		}

		return aiMat;
	}

	aiMesh* convertMesh(Mesh* mesh) 
	{
		const Mesh& ms = *mesh;

		aiMesh* aiMs = new aiMesh();

		GLuint idxOfMat = std::distance(src_md->materials.begin(), find(src_md->materials.begin(), src_md->materials.end(), ms.material));
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

	void recursiveConvertTree(aiNode* to, const Model::Node& from)
	{
		aiNode* node = new aiNode();
		std::vector<Mesh*>& modelMeshes = src_md->meshes;
		node->mTransformation = toAi(from.transformation);

		const size_t nrMeshes = from.meshes.size();
		node->mNumMeshes = nrMeshes;
		node->mMeshes = new unsigned int[nrMeshes];
		for( size_t i=0; i<nrMeshes; i++ ) {
			node->mMeshes[i] = std::distance(modelMeshes.begin(), std::find(modelMeshes.begin(), modelMeshes.end(), from.meshes[i]));
		}

		const size_t nrChilds = from.childs.size();
		node->mNumChildren = nrChilds;
		node->mChildren = new aiNode*[nrChilds];
		for( size_t i=0; i< from.childs.size(); i++ ) {
			recursiveConvertTree(node->mChildren[i], from.childs[i]);
		}
	}

	aiScene* makeScene(Model* model)
	{
		const Model& md = *model;
		aiScene *scene = new aiScene();

		// only one marerial
		const GLuint nrMats = md.materials.size();
		scene->mNumMaterials = nrMats;
		scene->mMaterials = new aiMaterial*[nrMats];
		for( int i = 0; i<nrMats; i++ ) {
			scene->mMaterials[i] = convertMaterial(md.materials[i]);
		}
		
		const GLuint nrMeshes = md.meshes.size();
		scene->mNumMeshes = nrMeshes;
		scene->mMeshes = new aiMesh*[nrMeshes];
		for( int i=0; i<nrMeshes; i++ ) {
			scene->mMeshes[i] = convertMesh(md.meshes[i]);
		}
		
		recursiveConvertTree(scene->mRootNode, md.root);

		return scene;
	}
};

namespace lim
{
	int getNrExportFormats()
	{
		return nr_formats;
	}
	const aiExportFormatDesc *getExportFormatInfo(int idx)
	{
		if (formats[0] == nullptr){
			for (int i = 0; i < nr_formats; i++)
				formats[i] = aiGetExportFormatDescription(i);
		}
		if (idx < 0 || idx >= nr_formats)
			return nullptr;
		return formats[idx];
	}

	void exportModelToFile(std::string exportDir, Model *model, size_t pIndex)
	{
		namespace fs = std::filesystem;
		const aiExportFormatDesc *format = formats[pIndex];
		src_md = model;

		const size_t lastSlashPos = model->path.find_last_of("/\\");
		md_dir = model->path.substr(0, lastSlashPos) + "/";

		/* create path */
		exportDir += "/"+model->name; // todo : test
		fs::path createdPath(exportDir);
		if (!std::filesystem::is_directory(createdPath))
			fs::create_directories(createdPath);
		std::string newModelPath = exportDir + model->name.c_str() +'.'+format->fileExtension;

		/* export model */
		rst_scene = makeScene(model);
		
		if( aiExportScene(rst_scene, format->id, newModelPath.data(), rst_scene->mFlags)!=AI_SUCCESS )
		{
			log::err("failed export %s\n", newModelPath.data());
		}
		delete rst_scene;
		rst_scene = nullptr;


		/* ctrl cv texture */
		for( Texture* tex : model->textures_loaded )
		{
			std::string internalPath = tex->path.data()+lastSlashPos;
			std::string newTexPath = exportDir + internalPath;

			fs::path fromTexPath(tex->path);
			fs::path toTexPath(newTexPath);
			fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
			log::pure("copied texture: %s to %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
		}
	}
}