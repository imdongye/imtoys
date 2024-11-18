/*
	imdongye@naver.com
	fst: 2023-09-05
	lst: 2023-09-05

Usage:
    lim::log::pure("hello, world! %05d\n", 123);

Ref:
    ExampleAppLog in imgui_demo.cpp

Todo:
    1. err callback
    2. ,comma operator 출력
    3. 다른쓰레드에서 포메팅
    4. 가변길이 템플릿?

*/

#ifndef __tools_log_h_
#define __tools_log_h_

#include <string>


namespace lim
{
    namespace log
    {
        constexpr int LOG_SPRINTF_BUF_SIZE = 512;
        void pure(const char* format, ...);
        void info(const char* format, ...);
        void warn(const char* format, ...);
        void err(const char* format, ...);

        void glError(int line = -1);

        void reset();
        void exportToFile(const char* filename = "log.txt");

        void drawImGui();
    }
}

#endif