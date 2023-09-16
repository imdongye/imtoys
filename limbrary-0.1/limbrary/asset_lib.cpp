#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/model_view/material.h>
#include <limbrary/log.h>
#include <limbrary/model_view/code_mesh.h>

namespace lim
{
	AssetLib::AssetLib()
	{
		log::pure("init AssetLib\n");

		tex_to_quad_prog = new Program("texToQuad");
		tex_to_quad_prog->attatch("quad.vs").attatch("tex_to_quad.fs").link();

		gray_to_quad_prog = new Program("grayToQuad");
		gray_to_quad_prog->attatch("quad.vs").attatch("gray_to_quad.fs").link();
	
		depth_prog = new Program("depth");
		depth_prog->attatch("mvp.vs").attatch("depth.fs").link();

		red_prog = new Program("red");
		red_prog->attatch("quad.vs").attatch("red.fs").link();

		default_prog = new Program("defualt");
		default_prog->attatch("mvp.vs").attatch("red.fs").link();

		default_mat.prog = default_prog;


		screen_quad = code_mesh::genQuad(false, false);
		sphere = code_mesh::genSphere();
		cube = code_mesh::genCube();
	}
	AssetLib::~AssetLib()
	{
		log::pure("delete AssetLib\n");

		delete tex_to_quad_prog;
		delete gray_to_quad_prog;
		delete depth_prog;
		delete red_prog;
		delete default_prog;

		delete screen_quad;
		delete sphere;
		delete cube;
	}
	AssetLib& AssetLib::get()
	{
		if( !instance ) {
			instance = new AssetLib();
		}
		return *instance;
	}
	void AssetLib::destroy()
	{
		if( instance )
			delete instance;
		instance = nullptr;
	}
}