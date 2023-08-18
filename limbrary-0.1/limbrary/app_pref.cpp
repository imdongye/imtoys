#include "app_pref.h"
#include <fstream>
#include <nlohmann/json.h>
#include <vector>
#include <regex>
#include "utils.h"
#include "logger.h"

using Json = nlohmann::json;

namespace lim
{
	AppPref::AppPref()
	{
		std::string text = getStrFromFile(FILE_PATH);
		if( text.length()<1 ) {
			Logger::get()<<"length<1 so not make json\n";
			return;
		}
		Json ijson = Json::parse(text);
		// dump para is tab size, none is one line
		//Logger::get()<<"read "<<FILE_PATH<<Logger::endl<< ijson.dump(2) <<Logger::endl;

		recentModelPaths = ijson["recentModelPaths"];
	}
	/* detete copy & copy asignment singleton obj */
	AppPref::AppPref(const AppPref&)=delete;
	AppPref& AppPref::operator=(const AppPref&)=delete;

	AppPref& AppPref::get()
	{
		static AppPref app_pref;
		return app_pref;
	}
	void AppPref::clearData()
	{
		recentModelPaths.clear();
	}
	void AppPref::save()
	{
		Json ojson;
		//*********************

		if( recentModelPaths.size() > MAX_RECENT_MP_SIZE ) {
			auto end = recentModelPaths.end();
			auto begin = end-MAX_RECENT_MP_SIZE;
			recentModelPaths = std::vector<std::string>(begin, end);
		}
		ojson["recentModelPaths"] = recentModelPaths;

		//*********************
		std::ofstream ofile;
		try {
			ofile.open(FILE_PATH);
			ofile << std::setw(4) << ojson << std::endl;
			ofile.close();
		} catch( std::ifstream::failure& e ) {
			Logger::get()<<"[error] fail read : "<<FILE_PATH<<", what? "<<e.what();
		}

		//std::string temp = ojson.dump(2);
		//Logger::get()<<"write "<<FILE_PATH<<Logger::endl<<temp<<Logger::endl;
	}
	void AppPref::pushPathWithoutDup(const std::string_view path)
	{
		// 절대경로를 상대경로로
		std::filesystem::path ap(path.data());
		std::string rp = std::filesystem::relative(ap, std::filesystem::current_path()).u8string();
		std::replace(rp.begin(), rp.end(), '\\', '/');

		Logger::get()<<rp<<Logger::endll;
		//같은거 있으면 지우기
		auto samePathPos = std::find(recentModelPaths.begin(), recentModelPaths.end(), rp);
		if( samePathPos!=recentModelPaths.end() )
			recentModelPaths.erase(samePathPos);
		if( rp.size()<1 ) return;
		recentModelPaths.emplace_back(rp);
	}
}