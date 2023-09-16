//
//	2022-09-18 / im dong ye
// 
//	imitated PlayerPref in unity
//	c++11 singleton pettern
// 
//	usage : 
//	1. AppPref::get()
// 
//	Todo:
//	1. 파일관련 최적화 https://modoocode.com/306
//	2. 소멸자에서 save안됨
//

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
		//****** property ******//
		AppBase* app;
		const int MAX_RECENT_MP_SIZE = 10;
		std::vector<std::string> recent_model_paths;
		//todo: store last excute app idx
		int selected_app_idx = 0;
		std::string selected_app_name = "none";
	private:
		AppPref();
		/* detete copy & copy asignment singleton obj */
		AppPref(const AppPref&)=delete;
		AppPref &operator=(const AppPref&)=delete;
	public:
		static AppPref& get();
		void clearData();
		void saveToFile();
		void saveRecentModelPath(const std::string_view path);
	};
}

#endif
