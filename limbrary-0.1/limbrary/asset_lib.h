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
	class Program;
	class Mesh;
	struct Material;
	class TexBase;

	class AssetLib
	{
	public:
		//****** property ******//
		Program tex_to_quad_prog;
		Program depth_prog;
		
		Program default_prog;
		Material default_material;
		
		MeshQuad screen_quad; // only poss
		MeshSphere sphere;
		MeshCube cube;

	private:
		AssetLib(const AssetLib&)=delete;
		AssetLib& operator=(const AssetLib&)=delete;
		AssetLib();
		~AssetLib();
		inline static AssetLib* instance = nullptr;
	public:
		static void create();
		static AssetLib& get();
		static void destroy();
	};
}

#endif
