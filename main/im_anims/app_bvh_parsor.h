/*
    2024-08-04 / im dong ye

*/
#ifndef __app_bvh_parsor_h_
#define __app_bvh_parsor_h_

#include <limbrary/extension/appbase_canvas3d.h>

namespace lim
{
	class AppBvhParsor : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "bvh parsor";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_INFO = "hello, world";

	public:
		AppBvhParsor();
		~AppBvhParsor();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() const final;
		virtual void canvasImGui() final;
		virtual void dndCallback(int count, const char **paths) final;
	};
}

#endif
