#include <limbrary/app_pref.h>
#include <limbrary/utils.h>
#include <limbrary/logger.h>
#include <fstream>
#include <nlohmann/json.h>
#include <vector>
#include <regex>

using Json = nlohmann::json;

namespace lim
{
	AppPref::AppPref()
	{
		std::string text = getStrFromFile(FILE_PATH);
		if( text.length()<1 ) {
			Log::get()<<"length<1 so not make json\n";
			return;
		}
		Json ijson = Json::parse(text);
		// dump para is tab size, none is one line
		//Log::get()<<"read "<<FILE_PATH<<Log::endl<< ijson.dump(2) <<Log::endl;

		recent_model_paths = ijson["recentModelPaths"];
	}

	AppPref& AppPref::get()
	{
		static AppPref app_pref;
		return app_pref;
	}
	void AppPref::clearData()
	{
		recent_model_paths.clear();
	}
	void AppPref::save()
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
		} catch( std::ifstream::failure& e ) {
			Log::get()<<"[error] fail read : "<<FILE_PATH<<", what? "<<e.what();
		}

		//std::string temp = ojson.dump(2);
		//Log::get()<<"write "<<FILE_PATH<<Log::endl<<temp<<Log::endl;
	}
	void AppPref::pushPathWithoutDup(const std::string_view path)
	{
		// 절대경로를 상대경로로
		std::filesystem::path ap(path.data());
		std::string rp = std::filesystem::relative(ap, std::filesystem::current_path()).u8string();
		std::replace(rp.begin(), rp.end(), '\\', '/');

		Log::get()<<rp<<Log::endll;
		//같은거 있으면 지우기
		auto samePathPos = std::find(recent_model_paths.begin(), recent_model_paths.end(), rp);
		if( samePathPos!=recent_model_paths.end() )
			recent_model_paths.erase(samePathPos);
		if( rp.size()<1 ) return;
		recent_model_paths.emplace_back(rp);
	}
}