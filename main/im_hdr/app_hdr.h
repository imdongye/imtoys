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
#include <im_hdr/color_awared_image.h>
#include <vector>

namespace lim
{
	class AppHdr: public AppBase
	{
	public: 
		inline static constexpr CStr APP_NAME = "color aware image viewer";
		inline static constexpr CStr APP_DIR  = "im_hdr";
		inline static constexpr CStr APP_DESCRIPTION = "loader/display";
	private:
		std::vector<ColorAwareImage*> imgs;
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
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
