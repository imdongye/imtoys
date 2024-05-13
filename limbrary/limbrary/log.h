/*

2023-09-05 / im dong ye

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

#ifndef __log_h_
#define __log_h_

namespace lim
{
    namespace log
    {
        void pure(const char* format, ...);
        void info(const char* format, ...);
        void warn(const char* format, ...);
        void err(const char* format, ...);

        void clear();
        void exportToFile(const char* filename = "log.txt");
        void drawViewer(const char* title, bool* pOpen = nullptr);
    }
}

#endif