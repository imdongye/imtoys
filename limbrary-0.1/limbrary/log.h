/*

2023-09-05 / im dong ye

usage:
lim::log::pure("hello, world! %05d\n", 123);

ref:
ExampleAppLog in imgui_demo.cpp

todo:
1. err call back
2. ,comma operator 출력

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