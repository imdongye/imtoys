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


#ifndef __model_loader_h_
#define __model_loader_h_

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace lim
{
	class ModelLoader
	{
	private:
		inline static Model* model=nullptr; // temp model
	public:
		static Model* loadFile(std::string_view _path, bool makeNormalized = false)
	private:
		static std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* ai_mat, aiTextureType ai_type)
		static Mesh* getParsedMesh(aiMesh* mesh, const aiScene* scene);
		static void parseNode(aiNode* node, const aiScene* scene, int depth_tex = 0);
	};
}

#endif
