//
//	2022-09-18 / im dong ye
// 
//	imitated PlayerPref in unity
//	c++11 singleton pettern
// 
//	usage : 
//	1. AppPref::get()
//

#ifndef APP_PREF_H
#define APP_PREF_H

#include <fstream>
#include <json/json.h>
#include <queue>
#include <vector>

namespace lim
{
	static std::string getStrFromFile(std::string_view path)
	{
		std::string text;
		std::ifstream ifile;
		//ifile.exceptions(std::ifstream::failbit|std::ifstream::badbit);
		try {
			std::stringstream ss;
			ifile.open(path.data());
			ss<<ifile.rdbuf(); // stream buffer
			ifile.close();
			text = ss.str();
		} catch( std::ifstream::failure& e ) {
			Logger::get()<<"[error] fail read : "<<path.data()<<", what? "<<e.what()<<Logger::endl;
		}
		if( text.length()<1 ) {
			Logger::get()<<"[error] length<1 : "<<path.data()<<Logger::endl;
		}
		return text;
	}
	static void setStrToFile(std::string_view path, std::string_view text)
	{
		std::ofstream ofile;
		try {
			ofile.open(path.data());
			ofile<<text;
			ofile.close();
		} catch( std::ifstream::failure& e ) {
			Logger::get()<<"[error] fail read : "<<path.data()<<", what? "<<e.what();
		}
	}

	class AppPref
	{
	private:
		using Json = nlohmann::json;
		const char* FILE_PATH = "app_pref.json";
	public:
		//****** property ******//
		const int MAX_RECENT_MP_SIZE = 10;
		std::vector<std::string> recentModelPaths;
	private:
		AppPref()
		{
			std::string text = getStrFromFile(FILE_PATH);
			if( text.length()<1 ) {
				Logger::get()<<"length<1 so not make json\n";
				return;
			}
			Json ijson = Json::parse(text);
			// dump para is tab size, none is one line
			Logger::get()<<"read "<<FILE_PATH<<Logger::endl<< ijson.dump(2)<<Logger::endl;

			recentModelPaths = ijson["recentModelPaths"];
		}
		AppPref(const AppPref&)=delete;
		AppPref &operator=(const AppPref&)=delete;
	public:
		static AppPref& get()
		{
			static AppPref app_pref;
			return app_pref;
		}
		void save()
		{
			Json ojson;
			//*********************

			if( recentModelPaths.size() > MAX_RECENT_MP_SIZE ) {
				auto end = recentModelPaths.end();
				auto begin = end-MAX_RECENT_MP_SIZE;
				std::vector<std::string> temp(begin, end);
			}
			else ojson["recentModelPaths"] = recentModelPaths;

			//*********************
			Logger::get()<<"write "<<FILE_PATH<<Logger::endl<< ojson.dump(2)<<Logger::endl;
			std::ofstream ofile;
			try {
				ofile.open(FILE_PATH);
				ofile << std::setw(4) << ojson << std::endl;
				ofile.close();
			} catch( std::ifstream::failure& e ) {
				Logger::get()<<"[error] fail read : "<<FILE_PATH<<", what? "<<e.what();
			}
		}
	};
}

#endif