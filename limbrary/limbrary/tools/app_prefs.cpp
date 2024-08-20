#include <limbrary/tools/app_prefs.h>
#include <limbrary/tools/general.h>
#include <limbrary/tools/log.h>
#include <fstream>
#include <nlohmann/json.h>
#include <vector>
#include <regex>

using Json = nlohmann::json;

namespace lim
{
	AppPrefs::AppPrefs()
	{
		log::pure("init AppPrefs\n");
		std::string text = readStrFromFile(FILE_PATH);
		if( text.length()<1 ) {
			log::err("length<1 so not make json\n");
			return;
		}
		Json ijson = Json::parse(text);
		// dump para is tab size, none is one line
		// log::pureln("read %s\n %s", FILE_PATH, ijson.dump(2));
		recent_model_paths = ijson["recentModelPaths"];
	}
	AppPrefs::~AppPrefs()
	{
		log::pure("delete AppPrefs\n");
	}

	void AppPrefs::create()
	{
		if( !instance )
			instance = new AppPrefs();
	}
	AppPrefs& AppPrefs::get()
	{
		return *instance;
	}
	void AppPrefs::destroy()
	{
		if( instance )
			delete instance;
		instance = nullptr;
	}

	void AppPrefs::clearData()
	{
		recent_model_paths.clear();
	}
	void AppPrefs::saveToFile()
	{
		Json ojson;
		//*********************

		if( recent_model_paths.size() > MAX_RECENT_MP_SIZE ) {
			auto end = recent_model_paths.end();
			auto begin = end-MAX_RECENT_MP_SIZE;
			recent_model_paths = std::vector<std::string>(begin, end);
		}
		ojson["recentModelPaths"] = recent_model_paths;

		//*********************
		std::ofstream ofile;
		try {
			ofile.open(FILE_PATH);
			ofile << std::setw(4) << ojson << std::endl;
			ofile.close();
		} catch( std::ofstream::failure& e ) {
			log::err("fail write : %s, what? %s \n", FILE_PATH, e.what());
		}

		//std::string temp = ojson.dump(2);
		//log::pure("write %s\n %s\n", FILE_PATH, temp);
	}
	void AppPrefs::saveRecentModelPath(const std::string_view path)
	{
		// 절대경로를 상대경로로
		std::filesystem::path ap(path.data());
		std::string rp = std::filesystem::relative(ap, std::filesystem::current_path()).u8string();
		std::replace(rp.begin(), rp.end(), '\\', '/');

		log::pure("%s\n", rp.c_str());
		//같은거 있으면 지우기
		auto samePathPos = std::find(recent_model_paths.begin(), recent_model_paths.end(), rp);
		if( samePathPos!=recent_model_paths.end() )
			recent_model_paths.erase(samePathPos);
		if( rp.size()<1 ) return;
		recent_model_paths.emplace_back(rp);
	}
}