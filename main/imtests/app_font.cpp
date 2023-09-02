#include "app_font.h"
#include <glad/glad.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
#include <limbrary/logger.h>

namespace lim
{
	AppFont::AppFont(): AppBase(1200, 780, APP_NAME)
	{
		//cpp11 raw string
		vert_shader = R"shader( 
			#version 410 core
			layout (location = 0) in vec3 aPos;

			uniform mat4 modelMat = mat4(1);
			uniform mat4 viewMat = mat4(1);
			uniform mat4 projMat= mat4(1);

			out vec3 wPos;

			void main(void) {
				wPos = (modelMat * vec4(aPos, 1.0)).xyz;
				gl_Position= projMat*viewMat*vec4(wPos,1.0);
			}
			)shader";
		vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
		assert(vg != NULL);

		fontBold = nvgCreateFont(vg, "sans-bold", "assets/fonts/NotoColorEmoji-Regular.ttf");
		assert(fontBold != -1);

		//ImGui::PushFont()
		ImGuiIO& io = ImGui::GetIO();
		font0 = io.Fonts->AddFontDefault();
		font1 = io.Fonts->AddFontFromFileTTF(u8"assets/fonts/NotoColorEmoji-Regular.ttf", 20.f);
		//static ImWchar ranges[] = {0x1, 0x1FFFF, 0};
		static ImFontConfig cfg;
		cfg.OversampleH = cfg.OversampleV = 1;
		cfg.MergeMode = true;
	}
	AppFont::~AppFont()
	{
		nvgDeleteGL3(vg);
	}
	void AppFont::update()
	{
		// clear backbuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		nvgBeginFrame(vg, fb_width, fb_height, pixel_ratio);
		nvgSave(vg);

		NVGpaint bg;
		char icon[8];
		float cornerRadius = 4.f;

		nvgFontSize(vg, 15.f);
		nvgFontFace(vg, "sans-bold");
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

		nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
		nvgText(vg, 100, 100, "나노vg💕", NULL);
		nvgRestore(vg);
		nvgEndFrame(vg);
	}
	void AppFont::renderImGui()
	{
		ImGui::Begin("asdf");
		ImGui::PushFont(font1);
		ImGui::Text(u8"asdf안녕하세요😊");
		ImGui::PopFont();
		ImGui::End();
	}
	void AppFont::keyCallback(int key, int scancode, int action, int mods)
	{
		Logger::get()<<ImGui::GetFrameHeight();
	}
	void AppFont::cursorPosCallback(double xPos, double yPos)
	{
		static double xOld, yOld, xOff, yOff=0;
		xOff = xPos - xOld;
		yOff = yOld - yPos;

		xOld = xPos;
		yOld = yPos;
	}
}
