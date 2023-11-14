/*

2022-12-30 / im dong ye

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
	class Program;
	class Mesh;
	struct Material;
	class Texture;

	class AssetLib
	{
	private:
		Program default_prog;
	public:
		//****** property ******//
		AppBase* app;

		Program tex_to_quad_prog;
		Program depth_prog;
		
		Material default_material;
		
		MeshQuad screen_quad; // only poss
		MeshSphere sphere;
		MeshCube cube;

	private:
		inline static AssetLib* instance = nullptr;
		AssetLib(const AssetLib&)=delete;
		AssetLib& operator=(const AssetLib&)=delete;
		AssetLib();
		~AssetLib();
	public:
		static void create(AppBase* app);
		static AssetLib& get();
		static void destroy();
	};
}

#endif
