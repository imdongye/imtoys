#include "app_nano.h"
#include <glad/glad.h>
#define NANOVG_GL3
#include <nanovg_gl.h>
#include <limbrary/log.h>
#include <imgui.h>

namespace lim
{
	AppNano::AppNano(): AppBase(1400, 780, APP_NAME)
	{
		vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
		assert(vg != NULL);

		for( int i=0; i<=10; i++ ) {
			glm::vec2 p = {i/10.f*fb_width, fb_height*0.5f};
			srcPts.push_back(p);
			samplePts.push_back(p);
		}
	}
	AppNano::~AppNano()
	{
		nvgDeleteGL3(vg);
	}
	void AppNano::drawEyes(NVGcontext* vg, float x, float y, float w, float h, float mx, float my, float t)
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
	void AppNano::update()
	{
		// clear backbuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		nvgBeginFrame(vg, fb_width, fb_height, pixel_ratio);

		nvgSave(vg);

		nvgBeginPath(vg);
		nvgMoveTo(vg, samplePts[0].x, samplePts[0].y);
		for( auto i=1; i<samplePts.size(); i++ ) {
			nvgLineTo(vg, samplePts[i].x, samplePts[i].y);
		}
		nvgStrokeColor(vg, nvgRGBAf(0, .3f, 1, 1));
		nvgStrokeWidth(vg, 2);
		nvgStroke(vg);

		nvgRestore(vg);

		for( auto i=0; i<srcPts.size(); i++ )
			if( i!= underPt ) {
				nvgBeginPath(vg);
				nvgCircle(vg, srcPts[i].x, srcPts[i].y, 5);
				nvgFillColor(vg, nvgRGBAf(1, 1, 0, .8f));
				nvgFill(vg);
			}
		if( underPt>=0 ) {
			nvgBeginPath(vg);
			nvgCircle(vg, srcPts[underPt].x, srcPts[underPt].y, 5);
			nvgFillColor(vg, nvgRGBAf(1, .1f, 0, .8f));
			nvgFill(vg);
		}

		//drawEyes(vg, 100, 100, 100, 100, mouse_pos.x, mouse_pos.y, glfwGetTime());

		nvgEndFrame(vg);
	}
	void AppNano::updateImGui()
	{
	}
	void AppNano::mouseBtnCallback(int btn, int action, int mod)
	{
		if( action == GLFW_PRESS && underPt>0 ) {
			initialOffset = mouse_pos - srcPts[underPt];
			isDragging = true;
		}
		else {
			isDragging = false;
		}
	}
	void AppNano::keyCallback(int key, int scancode, int action, int mods)
	{
	}
	void AppNano::cursorPosCallback(double xPos, double yPos)
	{
		if( !isDragging ) {
			underPt = -1;
			for( int i=0; i<srcPts.size(); i++ ) {
				if( glm::length(srcPts[i]-mouse_pos)<6 ) {
					underPt = i;
				}
			}
		}

		if( isDragging ) {
			srcPts[underPt] = mouse_pos -initialOffset;
			samplePts[underPt] = srcPts[underPt];
		}
	}
}