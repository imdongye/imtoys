/*

2022-12-30 / im dong ye

utils : AssetLib의 라이프사이클을 빌린 툴

*/

#ifndef __asset_lib_h_
#define __asset_lib_h_

#include <glad/glad.h>
#include "program.h"
#include "model_view/material.h"
#include "model_view/mesh_maked.h"

namespace lim
{
	class AppBase;

	class AssetLib
	{
	private:
		Program default_prog;
	public:
		//****** property ******//
		AppBase* app;

		Program tex_to_quad_prog;
		Program tex3d_to_quad_prog;
		Program depth_prog, ndv_prog;
		
		Material default_material;
		
		MeshQuad screen_quad; // only poss
		MeshSphere sphere;
		MeshSphere small_sphere;
		MeshCube cube;

	private:
		inline static AssetLib* instance = nullptr;
		AssetLib(const AssetLib&)=delete;
		AssetLib& operator=(const AssetLib&)=delete;
		AssetLib();
		~AssetLib();
	public:
		static void create(AppBase* app);
		static void destroy();
		static AssetLib& get();
	};

	
	namespace utils
	{
		void glErr( std::string_view msg );
		int getMsMaxSamples();
		void drawEnvSphere(const Texture& map, const glm::mat4& viewMat, const glm::mat4& projMat);
	}
}

#endif
