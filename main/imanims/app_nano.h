//
//  nonovg tester
//	2023-02-21 / im dong ye
//

#ifndef __app_nano_h_
#define __app_nano_h_

#include <limbrary/application.h>
#include <nanovg.h>
#include <vector>

namespace lim
{
	class AppNano : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "imnano";
		inline static constexpr CStr APP_DIR = "imanims";
		inline static constexpr CStr APP_DISC = "hello, world";

	private:
		enum
		{
			LAGLANGIAN,
			LINEAR,
			BEZIER,
			CATMULL,
			BSPLINE,
			CUBIC_NATURAL,
			CUBIC_CLOSED,
			CUBIC_NATURAL_QR,
		};

		enum
		{
			DRAW_LINES,
			DRAW_DOTS,
		};

		int curveType = LINEAR;
		int drawType = DRAW_LINES;
		bool closed = false;

		std::vector<glm::vec2> srcPts;
		std::vector<glm::vec2> samplePts;
		int underPt = -1;

		NVGcontext *vg = NULL;
		glm::vec2 initialOffset = {0, 0};
		bool isDragging = false;

	public:
		AppNano();
		~AppNano();

	private:
		void drawEyes(NVGcontext *vg, float x, float y, float w, float h, float mx, float my, float t);
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void mouseBtnCallback(int btn, int action, int mod) override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif
