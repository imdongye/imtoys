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
		inline static AssetLib* instance = nullptr;
		Program default_prog;

	public:
		//****** property ******//
		AppBase* app;

		Program tex_to_quad_prog;
		Program tex3d_to_quad_prog;
		Program depth_prog;
		Program ndv_prog;
		Program color_prog;
		
		MeshQuad screen_quad; // only poss
		MeshSphere sphere;
		MeshSphere small_sphere;
		MeshCube cube;
		MeshCylinder thin_cylinder;

	private:
		AssetLib(const AssetLib&)=delete;
		AssetLib& operator=(const AssetLib&)=delete;
		AssetLib(AssetLib&&)=delete;
		AssetLib& operator=(AssetLib&&)=delete;
		
		AssetLib();
		~AssetLib();
		
	public:
		static void create(AppBase* app);
		static void destroy();
		static AssetLib& get();
	};

	// Todo 
	namespace utils
	{
		void glErr( std::string_view msg );
		int getMsMaxSamples();
		void drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj);
	}
}

#endif
