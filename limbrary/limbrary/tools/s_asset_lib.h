/*

	2022-12-30 / im dong ye

Note:
	Singleton

*/

#ifndef __s_s_asset_lib_h_
#define __s_s_asset_lib_h_

#include <glad/glad.h>
#include "mecro.h"
#include "../program.h"
#include "../model_view/material.h"
#include "../model_view/mesh_maked.h"
#include "../viewport.h"

namespace lim
{
	class AppBase;

	class AssetLib : public SingletonDynamic<AssetLib>
	{
	public:
		//****** property ******//
		MeshQuad screen_quad; // only poss
		MeshPlane ground_quad;
		MeshSphere sphere;
		MeshSphere small_sphere; // no uv
		MeshCylinder thin_cylinder;
		MeshEnvSphere env_sphere;

		Viewport texture_viewer;
		
		
		Program prog_tex_to_quad;
		Program prog_tex3d_to_quad;
		Program prog_shadow_static;
		Program prog_shadow_skinned;
		Program prog_ndv;
		Program prog_color;
		Program prog_env;


	private:
		friend SingletonDynamic<AssetLib>;
		AssetLib();
		~AssetLib();
	};
}

#endif
