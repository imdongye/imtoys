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
#include <limbrary/tools/general.h>

using namespace std;


namespace 
{
    string logger_window_name;
    string buf;
    char line_buf[512];
    vector<int> line_offsets = {0};
    ImGuiTextFilter filter;

    bool is_auto_scroll_on = true;
    bool is_time_stamp_on = false;
}

static void appendHead(string_view level)
{
    if(is_time_stamp_on)
        stbsp_sprintf(line_buf, "[%05d] ", ImGui::GetFrameCount());
    else
        line_buf[0] = '\0';
    strcat(line_buf, level.data());
    
    cout<<line_buf;
    buf.append(line_buf);
}

static void appendfv(const char* fmt, va_list args)
{
    stbsp_vsprintf(line_buf, fmt, args);

    cout<<line_buf;
    buf.append(line_buf);
}

static void addFindedOffsets(int start, int end)
{
    for( int i=start; i<end; i++ ) {
        if( buf[i] == '\n')
            line_offsets.push_back(i+1);
    }
}


void lim::log::pure(const char* fmt, ...)
{
    appendHead("");
    int start = buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, buf.size());
}
void lim::log::info(const char* fmt, ...)
{
    appendHead("[info] ");
    int start = buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, buf.size());
}
void lim::log::warn(const char* fmt, ...)
{
    appendHead("[warn] ");
    int start = buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, buf.size());
}
void lim::log::err(const char* fmt, ...)
{
    appendHead("[errr] ");
    int start = buf.size();
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
    addFindedOffsets(start, buf.size());
}

void lim::log::glError(int line) {
    GLenum err = glGetError();
    
    const char* msg = nullptr;
    switch( err ) {
        case GL_INVALID_ENUM:                   msg = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                  msg = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:              msg = "GL_INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                 msg = "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:                msg = "GL_STACK_UNDERFLOW"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
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


void lim::log::reset()
{
    logger_window_name = fmtStrToBuf("Logger##%s", AppBase::g_app_name);
    buf.clear();
    buf.reserve(1024);
    line_offsets.clear();
    line_offsets.push_back(0);
}
void lim::log::exportToFile(const char* filename)
{
    string path = "exports/logs/";

    filesystem::path createdPath(path);
    if (!std::filesystem::is_directory(createdPath))
        filesystem::create_directories(createdPath);

    path += filename;
    ofstream file(path.c_str());
    if(file.is_open()){
        file.write(buf.c_str(),buf.size());
    }
    log::pure("export log to %s", path.c_str());
}
void lim::log::drawViewer()
{
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
        const char* bufStart = buf.c_str();
        const char* bufEnd = bufStart + buf.size();
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