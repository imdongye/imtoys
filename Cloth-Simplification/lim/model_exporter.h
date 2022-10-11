//
//  2022-09-27 / im dong ye
// 
//	edit from : https://github.com/assimp/assimp/issues/203
//

#ifndef MODEL_EXPORTER_H
#define MODEL_EXPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace lim
{
	class ModelExporter
	{
	public:
		inline static Assimp::Exporter exporter;
		inline static GLuint nr_formats = exporter.GetExportFormatCount();
	public:
		static void exportModel(std::string_view path, Model* model, size_t pIndex)
		{
			namespace fs = std::filesystem;
			/* create path */
			const aiExportFormatDesc* format = ModelExporter::getFormatInfo(pIndex);
			std::string notcreated(path);
			notcreated += model->name+"/";
			fs::path created_path(notcreated);
			if( !std::filesystem::is_directory(created_path) )
				fs::create_directories(created_path);
			notcreated += fmToStr("%s.%s",  model->name.c_str(), format->fileExtension);
			
			/* export model */
			aiScene* scene = makeScene(model);
			aiReturn ret = exporter.Export(scene, format->id, notcreated.data(), scene->mFlags);
			const char* error = exporter.GetErrorString();
			if(strlen(error)>0) Logger::get()<<"[error::exporter] "<<error <<Logger::endl;

			/* ctrl cv texture */
			
			fs::path textureBasePath(model->directory);
			for( Texture& tex : model->textures_loaded ) {
				fs::path fromTexPath(model->directory+tex.path);
				fs::path toTexPath(created_path.string()+tex.path);
				fs::copy(fromTexPath, toTexPath, fs::copy_options::skip_existing);
				Logger::get().log("%s %s\n", fromTexPath.string().c_str(), toTexPath.string().c_str());
			}
		}
		static const aiExportFormatDesc* getFormatInfo(size_t pIndex)
		{
			return exporter.GetExportFormatDescription(pIndex);
		}
	private:
		static aiScene* makeScene(Model* model)
		{
			const GLuint nr_meshes = model->meshes.size();
			aiScene* scene = new aiScene();
			aiNode* node = new aiNode();

			// only one marerial
			const GLuint nr_mat = model->aiNumMats;
			scene->mNumMaterials = nr_mat;
			scene->mMaterials = new aiMaterial*[nr_mat];
			for( int i=0; i<nr_mat; i++ ) {
				scene->mMaterials[i] = new aiMaterial();
				aiMaterial* mat;
				aiMaterial::CopyPropertyList(scene->mMaterials[i], (aiMaterial*)(model->aiMats[i]));
			}

			scene->mMeshes = new aiMesh*[nr_meshes] { nullptr, };
			scene->mNumMeshes = nr_meshes;

			scene->mRootNode = node;
			node->mMeshes = new unsigned int[nr_meshes] { 0, };
			node->mNumMeshes = nr_meshes;

			for( GLuint i=0; i<nr_meshes; i++ ) {
				Mesh* mesh = model->meshes[i];
				aiMesh* ai_mesh = new aiMesh();
				scene->mMeshes[i] = ai_mesh;

				const GLuint nr_verts = mesh->vertices.size();
				ai_mesh->mNumVertices = nr_verts;
				ai_mesh->mVertices = new aiVector3D[nr_verts];
				ai_mesh->mNormals = new aiVector3D[nr_verts];
				ai_mesh->mNumUVComponents[0] = nr_verts;
				ai_mesh->mTextureCoords[0] = new aiVector3D[nr_verts];
				ai_mesh->mMaterialIndex = mesh->aiMatIdx;

				for( int j=0; j<nr_verts; j++ ) {
					const auto& v = mesh->vertices[j];
					const auto& p = v.p;
					const auto& n = v.n;
					const auto& uv = v.uv;

					ai_mesh->mVertices[j] = aiVector3D(p.x, p.y, p.z);
					ai_mesh->mNormals[j] = aiVector3D(n.x, n.y, n.z);
					ai_mesh->mTextureCoords[0][j] = aiVector3D(uv.x, uv.y, 0);
				}

				const GLuint nr_indices = mesh->indices.size();
				const GLuint nr_tris = nr_indices/3;
				if( nr_indices%3 !=0 ) Logger::get().log("[error] not triangle mesh in exporter");
				ai_mesh->mNumFaces = nr_tris;
				ai_mesh->mFaces = new aiFace[nr_tris];

				for( GLuint j=0; j<nr_tris; j++ ) {
					const GLuint base=3*j;
					aiFace& face = ai_mesh->mFaces[j];
					face.mNumIndices = 3;
					face.mIndices = new unsigned int[3];
					for( GLuint k=0; k<3; k++ )
						face.mIndices[k] = mesh->indices[base+k];
				}
			}
			return scene;
		}
	};
}
#endif