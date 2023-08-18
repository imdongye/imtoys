#include "logger.h"
#include "GLFW/glfw3.h"

namespace lim
{
	Logger::Logger():windowName("Logger##log0"),simpTime(0.0), buffer{0}, autoScroll(true), addTimeStamp(false)
	{
		lines.emplace_back("");
	};

	Logger& Logger::get(int mode=0)
	{
		static Logger logger;
		if( mode==1 ) {
			static const char *__color_start = "[0;31;40m";
			static const char *__color_end = "[0m";
			printf("%s%s%s", __color_start, "[error]", __color_end);
		}

		return logger;
	}
	void Logger::drawImGui()
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
		clipper.Begin(lines.size());
		while( clipper.Step() )
			for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
				ImGui::TextUnformatted(lines[i].c_str());
		ImGui::PopStyleVar();

		if( autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
		ImGui::End();
	}
	Logger& Logger::log(FILE* stream, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vsprintf(buffer, format, args);
		va_end(args);

		fprintf(stream, "%s", buffer);
		seperate_and_save();
		return *this;
	}
	Logger& Logger::log(const char* format, ...)
	{
		va_list ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		va_end(ap);

		printf("%s", buffer);
		seperate_and_save();
		return *this;
	}
	Logger& Logger::operator<<(const int n)
	{
		return log("%d", n);
	}
	Logger& Logger::operator<<(const unsigned int n)
	{
		return log("%ld", n);
	}
	Logger& Logger::operator<<(const float f)
	{
		return log("%f", f);
	}
	Logger& Logger::operator<<(const double f)
	{
		return log("%lf", f);
	}
	Logger& Logger::operator<<(const char c)
	{
		return log("%c", c);
	}
	Logger& Logger::operator<<(const char* str)
	{
		return log("%s", str);
	}
	Logger& Logger::operator<<(const std::string& str)
	{
		return log("%s", str.c_str());
	}
	Logger& Logger::operator<<(Logger& (*fp)(Logger&))
	{
		return fp(*this);
	}
	Logger& Logger::endl(Logger& ref)
	{
		return ref.log("\n");
	}
	Logger& Logger::endll(Logger& ref)
	{
		return ref.log("\n\n");
	}
	void Logger::seperate_and_save()
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