#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/s_save_file.h>

#include <limbrary/application.h>
#include <limbrary/tools/general.h>
#include <limbrary/tools/log.h>

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
	, texture_viewer("replace before drawImGui", new FramebufferNoDepth(3,32))
{
	screen_quad.initGL(true);
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


// void SaveFile::saveToFile()
// {
// 	Json ojson;
// 	//*********************

// 	if( recent_model_paths.size() > MAX_RECENT_MP_SIZE ) {
// 		auto end = recent_model_paths.end();
// 		auto begin = end-MAX_RECENT_MP_SIZE;
// 		recent_model_paths = std::vector<std::string>(begin, end);
// 	}
// 	ojson["recentModelPaths"] = recent_model_paths;

// 	//*********************
// 	std::ofstream ofile;
// 	try {
// 		ofile.open(FILE_PATH);
// 		ofile << std::setw(4) << ojson << std::endl;
// 		ofile.close();
// 	} catch( std::ofstream::failure& e ) {
// 		log::err("fail write : %s, what? %s \n", FILE_PATH, e.what());
// 	}

// 	//std::string temp = ojson.dump(2);
// 	//log::pure("write %s\n %s\n", FILE_PATH, temp);
// }
// void SaveFile::saveRecentModelPath(const std::string_view path)
// {
// 	// 절대경로를 상대경로로
// 	std::filesystem::path ap(path.data());
// 	std::string rp = std::filesystem::relative(ap, std::filesystem::current_path()).u8string();
// 	std::replace(rp.begin(), rp.end(), '\\', '/');

// 	log::pure("%s\n", rp.c_str());
// 	//같은거 있으면 지우기
// 	auto samePathPos = std::find(recent_model_paths.begin(), recent_model_paths.end(), rp);
// 	if( samePathPos!=recent_model_paths.end() )
// 		recent_model_paths.erase(samePathPos);
// 	if( rp.size()<1 ) return;
// 	recent_model_paths.emplace_back(rp);
// }