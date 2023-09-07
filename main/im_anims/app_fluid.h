//
//  fluid simulation
//	2023-2-28 / im dong ye
//
//

#ifndef __app_fluid_h__
#define __app_fluid_h__

#include <limbrary/application.h>
#include <limbrary/viewport.h>
#include <vector>

namespace lim
{
	class AppFluid : public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "fluid tester";
		inline static constexpr const char *APP_DIR  = "im_anims";
		inline static constexpr const char *APP_DISC = "hello, world";

	private:
		Viewport *vp;
		Framebuffer *fb;

		std::vector<glm::vec2> points;

	public:
		AppFluid();
		~AppFluid();

	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void framebufferSizeCallback(int w, int h) override;
		virtual void mouseBtnCallback(int button, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif