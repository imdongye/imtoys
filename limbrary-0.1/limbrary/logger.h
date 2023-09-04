//
//	2022-09-17 / im dong ye
// 
//	usage : 
//	1. Log::get().attach("%d\n", a);
//	2. Log::get()<<"asdf"<<Log::endl;
// 
//	TODO list:
//  0. 분리
//  0. logger: string 저장, txt출력
//  0. log_viewer: imgui 뷰어
//	1. color error text
//	2. 하나의 문자열을 가지고 텍스트를 추가하며 라인의 끝 위치를 저장하게
//	3. json dump출력시 append에서 segfault발생 utf8문제인가?
//

#ifndef __logger_h_
#define __logger_h_

#include <string>
#include <vector>
#include <stdarg.h>


namespace lim
{
	class Log
	{
	public:
		enum LOG_LEVEL {
			LL_NOTE,
			LL_WARN,
			LL_ERR,
		};
	private:
		/* 고정 상수식이기에 static으로 data메모리에 있어야함 */
		static constexpr int BUFFER_SIZE = 512;
		char buffer[BUFFER_SIZE] = {0};
	public:
		std::string windowName="Log##log0";
		std::vector<std::string> lines;
		bool autoScroll = true;
		bool addTimeStamp = false;
	private:
		Log();
		Log(const Log&)=delete;
		Log &operator=(const Log&)=delete;
	public:
		static Log& get(LOG_LEVEL lev=LL_NOTE);
		void drawImGui();
		Log& log(FILE* stream, const char* format, ...);
		Log& log(const char* format, ...);
		Log& operator<<(const int n);
		Log& operator<<(const unsigned int n);
		Log& operator<<(const float f);
		Log& operator<<(const double f);
		Log& operator<<(const char c);
		Log& operator<<(const char* str);
		Log& operator<<(const std::string& str);
		Log& operator<<(Log& (*fp)(Log&));
		static Log& endl(Log& ref);
		static Log& endll(Log& ref);
	private:
		void seperate_and_save();
	};
}

#define LIM_LOG(...) lim::Log::get().printf(__VA_ARGS__)

#endif