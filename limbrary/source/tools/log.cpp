#include <limbrary/tools/log.h>
#include <glad/glad.h>
#include <stdarg.h>
#include <stb_sprintf.h>
#include <imgui.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limbrary/application.h>
#include <limbrary/using_in_cpp/std.h>
#include <limbrary/tools/text.h>

using namespace lim;

namespace 
{
    bool is_viewer_visible = false;
    string logger_window_name;
    string log_buf;
    vector<int> line_offsets = {0};
    ImGuiTextFilter filter;

    bool is_auto_scroll_on = true;
    bool is_time_stamp_on = false;
}

static void appendHead(const char* level=nullptr)
{
	const char* str;
    if(is_time_stamp_on) {
        str = fmtStrToBuf("%05d:", ImGui::GetFrameCount);
		log_buf.append(str);
		cout<<str;
    }
    if(level) {
        log_buf.append(level);
		cout<<level;
    }
}

static void appendfv(const char* fmt, va_list args)
{
    static char buf[log::LOG_SPRINTF_BUF_SIZE];
    stbsp_vsprintf(buf, fmt, args);
    cout<<buf;
    log_buf.append(buf);
}

static void addFindedOffsets(int start, int end)
{
    for( int i=start; i<end; i++ ) {
        if( log_buf[i] == '\n') {
            line_offsets.push_back(i+1);
		}
    }
}


void log::pure(const char* fmt, ...)
{
    appendHead();
    int start = log_buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, log_buf.size());
}
void log::info(const char* fmt, ...)
{
    appendHead("[info] ");
    int start = log_buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, log_buf.size());
}
void log::warn(const char* fmt, ...)
{
    appendHead("[warn] ");
    int start = log_buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, log_buf.size());
}
void log::err(const char* fmt, ...)
{
    appendHead("[errr] ");
    int start = log_buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, log_buf.size());
}

void log::glError(int line) {
    GLenum err = glGetError();
    
    const char* msg = nullptr;
    switch( err ) {
        case GL_INVALID_ENUM:                   msg = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                  msg = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:              msg = "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
#ifndef __APPLE__
        case GL_STACK_OVERFLOW:                 msg = "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:                msg = "GL_STACK_UNDERFLOW"; break;
#endif
        default:                                msg = "UNKNOWN"; break;
    }
    if( err == GL_NO_ERROR )
        log::pure("no gl error");
    else
        log::err("gl error %s(%d)", msg, err);

    if( line >= 0 )
        log::pure(" in line %d\n", line);
    else
        log::pure("\n");
}


void log::reset()
{
    is_viewer_visible = false;
    logger_window_name = fmtStrToBuf("Logger##%s", AppBase::g_app_name);
    log_buf.clear();
    log_buf.reserve(1024);
    line_offsets.clear();
    line_offsets.push_back(0);
}


void log::exportToFile(const char* filename)
{
    string path = "exports/logs/";

    std::filesystem::path createdPath(path);
    if (!std::filesystem::is_directory(createdPath))
        std::filesystem::create_directories(createdPath);

    path += filename;
    std::ofstream file(path.c_str());
    if(file.is_open()){
        file.write(log_buf.c_str(),log_buf.size());
    }
    log::pure("export log to %s", path.c_str());
}


void log::drawImGui()
{
    static bool is_opened = false;

    if( ImGui::IsKeyPressed(ImGuiKey_F2, false) ) {
        is_opened = !is_opened;
    }

    if( !is_opened ){
        return;
    }
    
    ImGui::Begin(logger_window_name.c_str());

    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &is_auto_scroll_on);
        ImGui::Checkbox("Time-stamp", &is_time_stamp_on);
        ImGui::EndPopup();
    }

    // Main window
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    bool doClear = ImGui::Button("Clear");
    ImGui::SameLine();
    if(ImGui::Button("Export")) {
        exportToFile("log.txt");
    }
    ImGui::SameLine();
    filter.Draw("Filter", -100.0f);

    ImGui::Separator();

    ImGuiWindowFlags scrolViewFlags = 0;
    //scrolViewFlags |= ImGuiWindowFlags_HorizontalScrollbar;
    if(ImGui::BeginChild("scrolling", ImVec2(0, 0), false, scrolViewFlags))
    {
        if (doClear)
            log::reset();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* bufStart = log_buf.c_str();
        const char* bufEnd = bufStart + log_buf.size();
        int nrLines = line_offsets.size();

        if (filter.IsActive())
        {
            for (int i = 0; i < nrLines; i++)
            {
                const char* lineStart = bufStart + line_offsets[i];
                const char* lineEnd = (i + 1 < nrLines) ? (bufStart + line_offsets[i + 1] - 1) : bufEnd;
                if (filter.PassFilter(lineStart, lineEnd))
                    ImGui::TextUnformatted(lineStart, lineEnd);
            }
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(nrLines);
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    const char* line_start = bufStart + line_offsets[i];
                    const char* line_end = (i + 1 < nrLines) ? (bufStart + line_offsets[i + 1] - 1) : bufEnd;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if(is_auto_scroll_on && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    
    ImGui::End();
}