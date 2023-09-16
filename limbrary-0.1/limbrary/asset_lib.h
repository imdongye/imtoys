//
//	2022-12-30 / im dong ye
//

#ifndef __asset_lib_h_
#define __asset_lib_h_

#include <glad/glad.h>
#include "program.h"
#include "model_view/mesh.h"
#include "model_view/material.h"

namespace lim
{
	class Program;
	class Mesh;
	class Material;

	class AssetLib
	{
	public:
		//****** property ******//
		Program* tex_to_quad_prog;
		Program* gray_to_quad_prog;
		Program* depth_prog;
		Program* red_prog;
		Program* default_prog;
		Material default_mat;
		
		Mesh* screen_quad; // only poss
		Mesh* sphere;
		Mesh* cube;

	private:
		inline static AssetLib* instance = nullptr;
		AssetLib();
		~AssetLib();
	private:
		/* detete copy & copy asignment singleton obj */
		AssetLib(const AssetLib&)=delete;
		AssetLib&operator=(const AssetLib&)=delete;
	public:
		static AssetLib& get();
		static void destroy();
	};
}

#endif
