#include <limbrary/logger.h>
#include <limbrary/utils.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <stdarg.h>
#include <iostream>

namespace lim
{
	Log::Log()
	{
		lines.emplace_back("");
	};

	Log& Log::get(LOG_LEVEL lev)
	{
		static Log logger;
		if( lev==LL_ERR ) {
			static const char *__color_start = "[0;31;40m";
			static const char *__color_end = "[0m";
			printf("%s%s%s", __color_start, "[error]", __color_end);
		}

		return logger;
	}
	void Log::drawImGui()
	{
		ImGui::Begin(windowName.c_str());

		ImGui::Checkbox("Auto-scroll", &autoScroll);
		ImGui::SameLine();
		ImGui::Checkbox("Add-timestamp", &addTimeStamp);
		ImGui::SameLine();
		if( ImGui::Button("Clear") ) {
			lines.clear();
			lines.push_back(" ");
		}
		ImGui::SameLine();
		ImGui::Text("        %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Separator();

		ImGui::BeginChild("Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		// Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the ImGuiListClipper helper.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGuiListClipper clipper;
		clipper.Begin((int)lines.size());
		while( clipper.Step() )
			for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
				ImGui::TextUnformatted(lines[i].c_str());
		ImGui::PopStyleVar();

		if( autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
		ImGui::End();
	}
	Log& Log::log(FILE* stream, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		va_end(args);

		fprintf(stream, "%s", buffer);
		seperate_and_save();
		return *this;
	}
	Log& Log::log(const char* format, ...)
	{
		va_list ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		va_end(ap);

		printf("%s", buffer);
		seperate_and_save();
		return *this;
	}
	Log& Log::operator<<(const int n)
	{
		return log("%d", n);
	}
	Log& Log::operator<<(const unsigned int n)
	{
		return log("%ld", n);
	}
	Log& Log::operator<<(const float f)
	{
		return log("%f", f);
	}
	Log& Log::operator<<(const double f)
	{
		return log("%lf", f);
	}
	Log& Log::operator<<(const char c)
	{
		return log("%c", c);
	}
	Log& Log::operator<<(const char* str)
	{
		return log("%s", str);
	}
	Log& Log::operator<<(const std::string& str)
	{
		return log("%s", str.c_str());
	}
	Log& Log::operator<<(Log& (*fp)(Log&))
	{
		return fp(*this);
	}
	Log& Log::endl(Log& ref)
	{
		return ref.log("\n");
	}
	Log& Log::endll(Log& ref)
	{
		return ref.log("\n\n");
	}
	void Log::seperate_and_save()
	{
		const char *start, *end;
		static char line_head[32];
		if( addTimeStamp )
			sprintf(line_head, "%3.2f | ", glfwGetTime());
		else
			line_head[0] = '\0';

		start = end = buffer;
		while( *end != '\0' ) {
			if( *end == '\n' ) {
				// if start==end then append empty
				lines.back().append(start, end);
				lines.push_back(line_head);

				start = end+1;
			}
			end++;
		}
		if( *start !='\n' )
			lines.back().append(start, end);
	}
}