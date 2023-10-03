/*

2022-09-18 / im dong ye
imitated PlayerPref in unity
c++11 singleton pettern

Usage : 
1. AppPref::get()

Todo:
1. 파일관련 최적화 https://modoocode.com/306
2. 소멸자에서 save안됨
3. 마지막실행 프로그램저장.

*/

#ifndef __app_pref_h_
#define __app_pref_h_

#include <string>
#include <vector>

namespace lim
{
	class AppBase;
	// singleton
	class AppPref
	{
	private:
		const char* FILE_PATH = "app_pref.json";
	public:
		AppBase* app;
		const int MAX_RECENT_MP_SIZE = 10;
		std::vector<std::string> recent_model_paths;
		//todo: store last excute app idx
		int selected_app_idx = 0;
		std::string selected_app_name = "none";
	private:
		AppPref(const AppPref&)=delete;
		AppPref& operator=(const AppPref&)=delete;
		AppPref();
		~AppPref();
	public:
		static AppPref& get();
		void clearData();
		void saveToFile();
		void saveRecentModelPath(const std::string_view path);
	};
}

#endif
