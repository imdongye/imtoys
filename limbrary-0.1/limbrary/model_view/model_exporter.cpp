//
//  2022-09-27 / im dong ye
//
//	edit from : https://github.com/assimp/assimp/issues/203
//
#include <limbrary/model_view/model_exporter.h>
#include <limbrary/logger.h>
#include <limbrary/utils.h>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <filesystem>


namespace
{
	using namespace lim;

	Assimp::Exporter exporter;
	const int nr_formats = (int)exporter.GetExportFormatCount();

	const aiExportFormatDesc *formats[32] = {
		nullptr,
	};

	aiScene *makeScene(Model *model)
	{
		const int nr_meshes = (int)model->meshes.size();
		aiScene *scene = new aiScene();
		aiNode *node = new aiNode();

		// only one marerial
		const GLuint nr_mat = model->ai_nr_mats;
		scene->mNumMaterials = nr_mat;
		scene->mMaterials = new aiMaterial *[nr_mat];
		for (int i = 0; i < nr_mat; i++)
		{
			scene->mMaterials[i] = new aiMaterial();
			//aiMaterial *mat; // todo???
			aiMaterial::CopyPropertyList(scene->mMaterials[i], (aiMaterial *)(model->ai_mats[i]));
		}

		scene->mMeshes = new aiMesh *[nr_meshes]
		{ nullptr, };
		scene->mNumMeshes = nr_meshes;

		scene->mRootNode = node;
		node->mMeshes = new unsigned int[nr_meshes]{
			0,
		};
		node->mNumMeshes = nr_meshes;

		for (GLuint i = 0; i < nr_meshes; i++)
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
				Log::get().log("[error] not triangle mesh in exporter");
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
		if (formats[0] == nullptr)
		{
			for (int i = 0; i < nr_formats; i++)
				formats[i] = exporter.GetExportFormatDescription(i);
		}
		if (idx < 0 || idx >= nr_formats)
			return nullptr;
		return formats[idx];
	}
	void exportModelToFile(std::string_view exportDir, Model *model, size_t pIndex)
	{
		namespace fs = std::filesystem;
		const aiExportFormatDesc *format = formats[pIndex];

		/* create path */
		std::string newModelDir(exportDir);
		newModelDir += model->name + "/";
		fs::path createdPath(newModelDir);
		if (!std::filesystem::is_directory(createdPath))
			fs::create_directories(createdPath);
		std::string newModelPath = newModelDir + fmToStr("%s.%s", model->name.c_str(), format->fileExtension);

		/* export model */
		aiScene *scene = makeScene(model);
		if(exporter.Export(scene, format->id, newModelPath.data(), scene->mFlags)!=AI_SUCCESS)
		{
			const char *error = exporter.GetErrorString();
			if (strlen(error) > 0)
				Log::get() << "[error::exporter] " << error << Log::endl;
		}

		/* ctrl cv texture */
		for (std::shared_ptr<Texture> tex : model->textures_loaded)
		{
			std::string newTexPath = newModelDir + std::string(tex->internal_model_path);

			fs::path fromTexPath(tex->path);
			fs::path toTexPath(newTexPath);
			fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
			Log::get().log("%s %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
		}
	}
}