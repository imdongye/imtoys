//
//	2022-09-17 / im dong ye
// 
//	usage : 
//	1. Logger::get().attach("%d\n", a);
//	2. Logger::get()<<"asdf"<<Logger::endl;		
// 
//	TODO list:
//	1. color error text
//	2. 하나의 문자열을 가지고 텍스트를 추가하며 라인의 끝 위치를 저장하게
//	3. json dump출력시 append에서 segfault발생 utf8문제인가?
//

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <stdarg.h>
#include <string.h>
#include "imgui_modules.h"

namespace lim
{
	class Logger
	{
	private:
		/* 고정 상수식이기에 static으로 data메모리에 있어야함 */
		static constexpr int BUFFER_SIZE = 256;
		char buffer[BUFFER_SIZE];
	public:
		std::vector<std::string> lines;
		double simpTime;
		bool autoScroll;
		bool addTimeStamp;
	private:
		Logger():simpTime(0.0), buffer{0}, autoScroll(true), addTimeStamp(false)
		{
			lines.emplace_back("");
		};
		Logger(const Logger&)=delete;
		Logger &operator=(const Logger&)=delete;
	public:
		static Logger& get()
		{
			static Logger logger;
			return logger;
		}
		void drawImGui()
		{
			ImGui::Begin("Logger");

			ImGui::Checkbox("Auto-scroll", &autoScroll);
			ImGui::SameLine();
			ImGui::Checkbox("Add-timestamp", &addTimeStamp);
			ImGui::SameLine();
			if( ImGui::Button("Clear") ) {
				lines.clear();
				lines.push_back(" ");
			}
			ImGui::Separator();

			ImGui::BeginChild("Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			// Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the ImGuiListClipper helper.
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGuiListClipper clipper;
			clipper.Begin(lines.size()-1);
			while( clipper.Step() )
				for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
					ImGui::TextUnformatted(lines[i].c_str());
			ImGui::PopStyleVar();

			if( autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();
			ImGui::End();
		}
		Logger& log(FILE* stream, const char* format, ...)
		{
			va_list args;
			va_start(args, format);
			vsprintf(buffer, format, args);
			va_end(args);

			fprintf(stream, "%s", buffer);
			seperate_and_save(buffer);
			return *this;
		}
		Logger& log(const char* format, ...)
		{
			va_list ap;
			va_start(ap, format);
			vsprintf(buffer, format, ap);
			va_end(ap);

			printf("%s", buffer);
			seperate_and_save(buffer);
			return *this;
		}
		Logger& operator<<(const int n)
		{
			return log("%d", n);
		}
		Logger& operator<<(const unsigned int n)
		{
			return log("%ld", n);
		}
		Logger& operator<<(const float f)
		{
			return log("%f", f);
		}
		Logger& operator<<(const char c)
		{
			return log("%c", c);
		}
		Logger& operator<<(const char* str)
		{
			return log("%s", str);
		}
		Logger& operator<<(const std::string& str)
		{
			return log("%s", str.c_str());
		}
		Logger& operator<<(Logger& (*fp)(Logger&))
		{
			return fp(*this);
		}
		static Logger& endl(Logger& ref)
		{
			return ref.log("\n");
		}
		static Logger& endll(Logger& ref)
		{
			return ref.log("\n\n");
		}
	private:
		void seperate_and_save(const char* buf)
		{
			const char *first, *last;
			const char const *end = strchr(buf, '\0');
			static char timeStamp[32];
			sprintf(timeStamp, "[%.2f] ", glfwGetTime());

			// case1. 그냥 한줄 출력
			if( !strchr(buf, '\n') ) {
				lines.back().append(buf);
				return;
			}

			first = buf;
			do {
				last = strchr(first, '\n');
				// case2. 개행 없이 끝날때
				if( !last ) {
					lines.back().append(first);
					break;
				}
				// case3. 마지막 개행
				else if( first == end ) {
					lines.push_back((addTimeStamp)?timeStamp:"");
					break;
				}
				// case4. 개행사이 한줄
				else {
					lines.back()+=std::string(first, last);
					lines.push_back((addTimeStamp)?timeStamp:"");
				}
				first = last+1;
			} while( last!=NULL && first<end );
		}
	};
}
#endif