//
//  nonovg tester
//	2023-02-21 / im dong ye
//

#ifndef APP_NANO_H
#define APP_NANO_H

#include "../limbrary/limclude.h"

#include <nanovg/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/nanovg_gl.h>

namespace lim
{
	class AppNano: public AppBase
	{
	public:
		inline static constexpr const char const *APP_NAME = "imnano";
		inline static constexpr const char const *APP_DIR = "imanims/";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:
		NVGcontext* vg = NULL;


	public:
		AppNano(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
			assert(vg != NULL);
		}
		~AppNano()
		{

		}
	private:

		void drawEyes(NVGcontext* vg, float x, float y, float w, float h, float mx, float my, float t)
		{
			NVGpaint gloss, bg;
			float ex = w *0.23f;
			float ey = h * 0.5f;
			float lx = x + ex;
			float ly = y + ey;
			float rx = x + w - ex;
			float ry = y + ey;
			float dx, dy, d;
			float br = (ex < ey ? ex : ey) * 0.5f;
			float blink = 1 - pow(sinf(t*0.5f), 200)*0.8f;

			bg = nvgLinearGradient(vg, x, y+h*0.5f, x+w*0.1f, y+h, nvgRGBA(0, 0, 0, 32), nvgRGBA(0, 0, 0, 16));
			nvgBeginPath(vg);
			nvgEllipse(vg, lx+3.0f, ly+16.0f, ex, ey);
			nvgEllipse(vg, rx+3.0f, ry+16.0f, ex, ey);
			nvgFillPaint(vg, bg);
			nvgFill(vg);

			bg = nvgLinearGradient(vg, x, y+h*0.25f, x+w*0.1f, y+h, nvgRGBA(220, 220, 220, 255), nvgRGBA(128, 128, 128, 255));
			nvgBeginPath(vg);
			nvgEllipse(vg, lx, ly, ex, ey);
			nvgEllipse(vg, rx, ry, ex, ey);
			nvgFillPaint(vg, bg);
			nvgFill(vg);

			dx = (mx - rx) / (ex * 10);
			dy = (my - ry) / (ey * 10);
			d = sqrtf(dx*dx+dy*dy);
			if( d > 1.0f ) {
				dx /= d; dy /= d;
			}
			dx *= ex*0.4f;
			dy *= ey*0.5f;
			nvgBeginPath(vg);
			nvgEllipse(vg, lx+dx, ly+dy+ey*0.25f*(1-blink), br, br*blink);
			nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
			nvgFill(vg);

			dx = (mx - rx) / (ex * 10);
			dy = (my - ry) / (ey * 10);
			d = sqrtf(dx*dx+dy*dy);
			if( d > 1.0f ) {
				dx /= d; dy /= d;
			}
			dx *= ex*0.4f;
			dy *= ey*0.5f;
			nvgBeginPath(vg);
			nvgEllipse(vg, rx+dx, ry+dy+ey*0.25f*(1-blink), br, br*blink);
			nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
			nvgFill(vg);

			gloss = nvgRadialGradient(vg, lx-ex*0.25f, ly-ey*0.5f, ex*0.1f, ex*0.75f, nvgRGBA(255, 255, 255, 128), nvgRGBA(255, 255, 255, 0));
			nvgBeginPath(vg);
			nvgEllipse(vg, lx, ly, ex, ey);
			nvgFillPaint(vg, gloss);
			nvgFill(vg);

			gloss = nvgRadialGradient(vg, rx-ex*0.25f, ry-ey*0.5f, ex*0.1f, ex*0.75f, nvgRGBA(255, 255, 255, 128), nvgRGBA(255, 255, 255, 0));
			nvgBeginPath(vg);
			nvgEllipse(vg, rx, ry, ex, ey);
			nvgFillPaint(vg, gloss);
			nvgFill(vg);
		}
		virtual void update() final
		{
			// clear backbuffer
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, fb_width, fb_height);
			glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			nvgBeginFrame(vg, fb_width, fb_height, pixel_ratio);

			float rect_w = 250.f, rect_h = 250.f;
			NVGcolor nvg_stroke_color = nvgRGBAf(0.f, 0.f, 0.f, 1.f);
			NVGcolor nvg_fill_color = nvgRGBAf(0.f, 1.f, 0.1f, 1.f);

			nvgBeginPath(vg);

			nvgRoundedRectVarying(
				vg,
				(float)win_width/2.f - rect_w/2.f, win_height/2.f - rect_h/2.f,
				rect_w, rect_h,
				30.f, 8.f, 30.f, 8.f
			);

			nvgFillColor(vg, nvg_fill_color);
			nvgFill(vg);
			nvgStrokeWidth(vg, 2.f);
			nvgStrokeColor(vg, nvg_stroke_color);
			nvgStroke(vg);

			drawEyes(vg, 100, 100, 100, 100, mouse_pos.x, mouse_pos.y, glfwGetTime());

			nvgEndFrame(vg);
		}
		virtual void renderImGui() final
		{
			//imgui_modules::DockSpaceOverViewport();

			//ImGui::ShowDemoWindow();

			//ImGui::Begin("test window");
			//ImGui::End();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		virtual void cursorPosCallback(double xPos, double yPos) final
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
