//
//  framework template
//	2023-3-9 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef APP_FONT_H
#define APP_FONT_H

#include <limbrary/limclude.h>

namespace lim
{
	class AppFont: public AppBase
	{
	public:
		inline static constexpr const char const *APP_NAME = "font";
		inline static constexpr const char const *APP_DIR = "imtests/";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:
		// c++11 Raw String
		std::string vert_shader = R"shader( 
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

		NVGcontext* vg = NULL;
		int fontNormal, fontBold, fontIcons, fontEmoji;
		ImFont* font1, *font0;
	public:
		AppFont(): AppBase(1280, 720, APP_NAME)
		{
			vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
			assert(vg != NULL);

			fontBold = nvgCreateFont(vg, "sans-bold", "common/font/NotoColorEmoji-Regular.ttf");
			assert(fontBold != -1);

			//ImGui::PushFont()
			ImGuiIO& io = ImGui::GetIO();
			font0 = io.Fonts->AddFontDefault();
			font1 = io.Fonts->AddFontFromFileTTF(u8"common/font/NotoColorEmoji-Regular.ttf", 20.f);
			static ImWchar ranges[] = {0x1, 0x1FFFF, 0};
			static ImFontConfig cfg;
			cfg.OversampleH = cfg.OversampleV = 1;
			cfg.MergeMode = true;
		}
		~AppFont()
		{
			nvgDeleteGL3(vg);
		}
	private:
		virtual void update() override
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
		virtual void renderImGui() override
		{
			ImGui::ShowDemoWindow();
			ImGui::Begin("asdf");
			ImGui::PushFont(font1);
			ImGui::Text(u8"asdf안녕하세요😊");
			ImGui::PopFont();
			ImGui::End();
		}
	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) override
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		virtual void cursorPosCallback(double xPos, double yPos) override
		{
			static double xOld, yOld, xOff, yOff=0;
			xOff = xPos - xOld;
			yOff = yOld - yPos;

			xOld = xPos;
			yOld = yPos;
		}
	};
}

#endif
