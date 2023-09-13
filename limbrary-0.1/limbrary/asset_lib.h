//
//	2022-12-30 / im dong ye
//

#ifndef __asset_lib_h_
#define __asset_lib_h_

#include <glad/glad.h>

namespace lim
{
	class Program;
	class Mesh;
	class Material;

	class AssetLib
	{
	public:
		//****** property ******//
		Program* gray_to_quad_prog;
		Program* tex_to_quad_prog;
		Program* depth_prog;
		Program* red_prog;
		GLuint quad_vbo = 0;
		GLuint quad_vao = 0;
		Mesh* quad;
		Program* defualt_prog;
		Material* default_mat;

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
