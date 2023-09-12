//
//  2022-09-27 / im dong ye
//
//	edit from : https://github.com/assimp/assimp/issues/203
//
#include <limbrary/model_view/model.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>
#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <filesystem>


namespace
{
	using namespace lim;

	const int nr_formats = (int)aiGetExportFormatCount();

	const aiExportFormatDesc *formats[32] = {
		nullptr,
	};

	aiMaterial* convertMaterial(Material* mat)
	{

	}

	aiMesh* convertMesh(Mesh* ms) 
	{
		Mesh *mesh = model->meshes[i];
		aiMesh *ai_mesh = new aiMesh();
		scene->mMeshes[i] = ai_mesh;

		const GLuint nr_verts = mesh->vertices.size();
		ai_mesh->mNumVertices = nr_verts;
		ai_mesh->mVertices = new aiVector3D[nr_verts];
		ai_mesh->mNormals = new aiVector3D[nr_verts];
		ai_mesh->mNumUVComponents[0] = nr_verts;
		ai_mesh->mTextureCoords[0] = new aiVector3D[nr_verts];
		ai_mesh->mMaterialIndex = mesh->ai_mat_idx;

		for (int j = 0; j < nr_verts; j++)
		{
			const auto &v = mesh->vertices[j];
			const auto &p = v.p;
			const auto &n = v.n;
			const auto &uv = v.uv;

			ai_mesh->mVertices[j] = aiVector3D(p.x, p.y, p.z);
			ai_mesh->mNormals[j] = aiVector3D(n.x, n.y, n.z);
			ai_mesh->mTextureCoords[0][j] = aiVector3D(uv.x, uv.y, 0);
		}

		const GLuint nr_indices = mesh->indices.size();
		const GLuint nr_tris = nr_indices / 3;
		if (nr_indices % 3 != 0)
			log::err("not triangle mesh in exporter");
		ai_mesh->mNumFaces = nr_tris;
		ai_mesh->mFaces = new aiFace[nr_tris];

		for (GLuint j = 0; j < nr_tris; j++)
		{
			const GLuint base = 3 * j;
			aiFace &face = ai_mesh->mFaces[j];
			face.mNumIndices = 3;
			face.mIndices = new unsigned int[3];
			for (GLuint k = 0; k < 3; k++)
				face.mIndices[k] = mesh->indices[base + k];
		}
	}

	aiNode* recursiveConvertTree(Model::Node* node)
	{

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
		
		scene->mRootNode = recursiveConvertTree(md.root);

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

		/* create path */
		exportDir += "/"+model->name; // todo : test
		fs::path createdPath(exportDir);
		if (!std::filesystem::is_directory(createdPath))
			fs::create_directories(createdPath);
		std::string newModelPath = exportDir + model->name.c_str() +'.'+format->fileExtension;

		/* export model */
		aiScene *scene = makeScene(model);
		
		if( aiExportScene(scene, format->id, newModelPath.data(), scene->mFlags)!=AI_SUCCESS )
		{
			log::err("failed export %s\n", newModelPath.data());
		}

		/* ctrl cv texture */
		for( TexBase* tex : model->textures_loaded )
		{
			const size_t lastSlashPos = model->path.find_last_of("/\\");
			std::string internalPath = tex->path.data()+lastSlashPos;
			std::string newTexPath = exportDir + std::string(internalPath);

			fs::path fromTexPath(tex->path);
			fs::path toTexPath(newTexPath);
			fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
			log::pure("%s %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
		}
	}
}