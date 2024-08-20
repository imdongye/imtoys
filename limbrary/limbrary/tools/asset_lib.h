/*

2022-12-30 / im dong ye

utils : AssetLib의 라이프사이클을 빌린 툴

*/

#ifndef __asset_lib_h_
#define __asset_lib_h_

#include <glad/glad.h>
#include "../program.h"
#include "../model_view/material.h"
#include "../model_view/mesh_maked.h"
#include "../viewport.h"

namespace lim
{
	class AppBase;

	class AssetLib
	{
	private:
		inline static AssetLib* instance = nullptr;

	public:
		//****** property ******//
		AppBase* app;

		Program prog_tex_to_quad;
		Program prog_tex3d_to_quad;
		Program prog_shadow_static;
		Program prog_shadow_skinned;
		Program prog_ndv;
		Program prog_color;

		
		MeshQuad screen_quad; // only poss
		MeshSphere sphere;
		MeshSphere small_sphere; // no uv
		MeshCylinder thin_cylinder;

		Viewport texture_viewer;

		Program prog_env;
		MeshEnvSphere env_sphere;

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



	// Todo ...
	namespace utils
	{
		void glErr( std::string_view msg );
		int getMsMaxSamples();
		void drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj);
	}
}

#endif
