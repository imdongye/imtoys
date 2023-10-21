#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/model_view/material.h>
#include <limbrary/log.h>
#include <limbrary/model_view/mesh_maked.h>

namespace lim
{
	AssetLib::AssetLib()
		: screen_quad(false, false)
	{
		log::pure("init AssetLib\n");

		tex_to_quad_prog.name = "texToQuad";
		tex_to_quad_prog.attatch("canvas.vs").attatch("canvas_color.fs").link();
	
		depth_prog.name = "depth";
		depth_prog.attatch("mvp.vs").attatch("depth.fs").link();


		default_prog.name = "defualt";
		default_prog.attatch("mvp.vs").attatch("ndv.fs").link();
		default_material.prog = &default_prog;
		default_material.set_prog = [](const Program&){};
	}
	AssetLib::~AssetLib()
	{
		log::pure("delete AssetLib\n");
	}
	void AssetLib::create(AppBase* _app)
	{
		if( !instance )
			instance = new AssetLib();
		instance->app = _app;
	}
	AssetLib& AssetLib::get()
	{
		return *instance;
	}
	void AssetLib::destroy()
	{
		if( instance )
			delete instance;
		instance = nullptr;
	}
}