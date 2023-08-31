//
//  for test hdr image.
//	2022-11-14 / im dong ye
// 
//	ref: http://regex.info/blog/photo-tech/color-spaces-page2
//
//	TODO list:
//
//

#ifndef __app_her_h_
#define __app_her_h_

#include <limbrary/application.h>
#include <limbrary/program.h>
#include <limbrary/viewport.h>

namespace lim
{
	class AppHdr: public AppBase
	{
	public: 
		inline static constexpr CStr APP_NAME = "color aware image viewer";
		inline static constexpr CStr APP_DIR = "imhdr";
		inline static constexpr CStr APP_DISC = "loader/display";
	private:
		CStr exportPath = "result/";

		std::vector<Program*> programs;
		std::vector<Viewport*> viewports;
	public:
		AppHdr();
		~AppHdr();
	private:
		void addImage(std::string_view path);
	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
		virtual void mouseBtnCallback(int btn, int action, int mods) override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
