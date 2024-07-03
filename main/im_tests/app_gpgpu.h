/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_gpgpu_h_
#define __app_gpgpu_h_

#include <limbrary/application.h>

namespace lim
{
	class AppGpgpu : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "gpgpu";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";


	public:
		AppGpgpu();
		~AppGpgpu();
		virtual void update() override;
		virtual void updateImGui() override;

		
	};
}

#endif
