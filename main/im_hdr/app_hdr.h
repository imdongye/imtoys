//
//  for test hdr image.
//	2022-11-14 / im dong ye
// 
//	ref: http://regex.info/blog/photo-tech/color-spaces-page2
//
//	Todo:
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
		inline static constexpr CStr APP_DIR  = "im_hdr/";
		inline static constexpr CStr APP_INFO = "loader/display";
	private:
		std::vector<ColorAwareImage*> imgs;
		std::vector<Program*> programs;
		std::vector<Viewport*> viewports;
	public:
		AppHdr();
		~AppHdr();
	private:
		void addImage(const char* path);
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
