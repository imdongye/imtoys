/*

2022-09-18 / im dong ye
imitated PlayerPref in unity
c++11 singleton pettern

Usage : 
1. AppPrefs::get()

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
	// singleton
	class AppPrefs
	{
	private:
		const char* FILE_PATH = "app_pref.json";
	public:
		const int MAX_RECENT_MP_SIZE = 10;
		std::vector<std::string> recent_model_paths;
	private:
		AppPrefs(const AppPrefs&)=delete;
		AppPrefs& operator=(const AppPrefs&)=delete;
		AppPrefs();
		~AppPrefs();
		inline static AppPrefs* instance = nullptr;
	public:
		static void create();
		static AppPrefs& get();
		static void destroy();

		void clearData();
		void saveToFile();
		void saveRecentModelPath(const std::string_view path);
	};
}

#endif
