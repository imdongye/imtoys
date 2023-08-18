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
	class AppPref
	{
	private:
		const char* FILE_PATH = "app_pref.json";
	public:
		//****** property ******//
		const int MAX_RECENT_MP_SIZE = 10;
		std::vector<std::string> recentModelPaths;
		//todo: store last excute app idx
		int selectedAppIdx = 0;
		std::string selectedAppName = "none";
	private:
		AppPref();
		/* detete copy & copy asignment singleton obj */
		AppPref(const AppPref&)=delete;
		AppPref &operator=(const AppPref&)=delete;
	public:
		static AppPref& get();
		void clearData();
		void save();
		void pushPathWithoutDup(const std::string_view path);
	};
}

#endif
