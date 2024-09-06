#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/s_save_file.h>

#include <limbrary/application.h>
#include <limbrary/tools/general.h>
#include <limbrary/tools/log.h>
#include <limbrary/using_in_cpp/std.h>

#include <fstream>

using Json = nlohmann::json;
using namespace lim;


AssetLib::AssetLib()
	: screen_quad(2.f, 2.f, false, false)
	, ground_quad(20.f, 20.f, 1, 1, true, true)
	, sphere(1.f, 50, 25, true, true)
	, small_sphere(0.2f, 8, 4,true, false)
	, thin_cylinder(0.1f)
	, env_sphere(80.f)
	, texture_viewer(fmtStrToBuf("TextureViewer##%s",AppBase::g_app_name), new FramebufferNoDepth(3,32))
{
	screen_quad.initGL(true);
	ground_quad.initGL(true);
	sphere.initGL(true);
	small_sphere.initGL(true);
	thin_cylinder.initGL(true);
	env_sphere.initGL(true);


	log::pure("AssetLib: created\n");

	prog_tex_to_quad.name = "texToQuad";
	prog_tex_to_quad.attatch("canvas.vs").attatch("canvas_color.fs").link();
		
	prog_tex3d_to_quad.name = "tex3dToQuad";
	prog_tex3d_to_quad.attatch("canvas.vs").attatch("tex3d_to_2d.fs").link();

	prog_shadow_static.name = "shadow_skinned";
	prog_shadow_static.attatch("mvp.vs").attatch("depth.fs").link();

	prog_shadow_skinned.name = "shadow_skinned";
	prog_shadow_skinned.attatch("mvp_skinned.vs").attatch("depth.fs").link();

	prog_ndv.name = "ndv";
	prog_ndv.attatch("mvp.vs").attatch("ndv.fs").link();

	prog_color.name = "red";
	prog_color.attatch("mvp.vs").attatch("color.fs").link();

	prog_env.name = "env";
	prog_env.attatch("mvp.vs").attatch("env.fs").link();
}

AssetLib::~AssetLib()
{
	log::pure("AssetLib: destroyed\n");
}








SaveFile::SaveFile()
{
	std::string text = readStrFromFile(FILE_PATH);
	if( text.empty() == false ) {
		jfile = Json::parse(text);

		const char* appName = AppBase::g_app_name;
		if( jfile[appName]!=nullptr ) {
			data = jfile[appName];
			log::pure("SaveFile: read app_save.json\n");
		}
	}
}
SaveFile::~SaveFile()
{
	// Todo: test
	saveToFile();
}



void SaveFile::saveToFile() const
{
	if( data.empty() ) {
		return;
	}

	const char* appName = AppBase::g_app_name;

	jfile[appName] = data;
	std::ofstream ofile;
	try {
		ofile.open(FILE_PATH);
		ofile << std::setw(4) << jfile << std::endl;
		ofile.close();
		log::pure("SaveFile: write app_save.json\n");
	} catch( std::ofstream::failure& e ) {
		log::err("fail write : %s, what? %s \n", FILE_PATH, e.what());
	}
}