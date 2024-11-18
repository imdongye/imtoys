/*
    data is for cur app
    jfile is for folder
*/

#include <limbrary/tools/save_file.h>
#include <limbrary/application.h>
#include <limbrary/tools/text.h>
#include <fstream>

using namespace lim;


// var definition
nlohmann::json save_file::data;


namespace {
	std::string file_path = "app_dir/app_save.json";
	nlohmann::json jfile;
}


void save_file::init()
{
	file_path = fmtStrToBuf("%sapp_save.json", AppBase::g_app_dir);
	std::string text = readStrFromFile(file_path.c_str());
	if( text.empty() == false ) {
		jfile = nlohmann::json::parse(text);

		const char* appName = AppBase::g_app_name;
		if( jfile[appName]!=nullptr ) {
			data = jfile[appName];
			log::pure("save_file: read app_save.json\n");
		}
	}
}

void save_file::deinit()
{
	saveToFile();
}

void save_file::saveToFile()
{
	if( data.empty() ) {
		return;
	}

	const char* appName = AppBase::g_app_name;

	jfile[appName] = data;
	std::ofstream ofile;
	try {
		ofile.open(file_path.c_str());
		ofile << std::setw(4) << jfile << std::endl;
		ofile.close();
		log::pure("save_file: write app_save.json\n");
	} catch( std::ofstream::failure& e ) {
		log::err("fail write : %s, what? %s \n", file_path.c_str(), e.what());
	}
}