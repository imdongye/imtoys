//
//	2022-09-17 / im dong ye
// 
//	usage : 
//	1. Logger::get().attach("%d\n", a);
//	2. Logger::get()<<"asdf"<<Logger::endl;		
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

#include <stdarg.h>
#include <string.h>
#include "utils.h"
#include "imgui_modules.h"


namespace lim
{
	class Logger
	{
	private:
		/* 고정 상수식이기에 static으로 data메모리에 있어야함 */
		static constexpr int BUFFER_SIZE = 512;
		char buffer[BUFFER_SIZE];
	public:
		std::string windowName;
		std::vector<std::string> lines;
		double simpTime;
		bool autoScroll;
		bool addTimeStamp;
	private:
		Logger();
		Logger(const Logger&)=delete;
		Logger &operator=(const Logger&)=delete;
	public:
		static Logger& get(int mode=0);
		void drawImGui();
		Logger& log(FILE* stream, const char* format, ...);
		Logger& log(const char* format, ...);
		Logger& operator<<(const int n);
		Logger& operator<<(const unsigned int n);
		Logger& operator<<(const float f);
		Logger& operator<<(const double f);
		Logger& operator<<(const char c);
		Logger& operator<<(const char* str);
		Logger& operator<<(const std::string& str);
		Logger& operator<<(Logger& (*fp)(Logger&));
		static Logger& endl(Logger& ref);
		static Logger& endll(Logger& ref);
	private:
		void seperate_and_save();
	};
}
#endif