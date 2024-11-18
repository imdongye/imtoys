/*
	imdongye@naver.com
	fst: 2022-12-30
	lst: 2024-11-19
*/

#ifndef __s_asset_lib_h_
#define __s_asset_lib_h_

#include <glad/glad.h>
#include "../3d/mesh.h"
#include "../program.h"
#include "../viewport.h"

namespace lim
{
	namespace asset_lib
	{
		void init();
		void deinit();

		extern const Mesh* screen_quad; // only poss
		extern const Mesh* big_plane;
		extern const Mesh* sphere;
		extern const Mesh* small_sphere; // no uv
		extern const Mesh* thin_cylinder;
		extern const Mesh* env_sphere;
		
		extern const Program* prog_tex_to_quad;
		extern const Program* prog_tex3d_to_quad;
		extern const Program* prog_shadow_static;
		extern const Program* prog_shadow_skinned;
		extern const Program* prog_ndv;
		extern const Program* prog_color;
		extern const Program* prog_env;
		
		extern Viewport* texture_viewer;
	}
}

#endif
