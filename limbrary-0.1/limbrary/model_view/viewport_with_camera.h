/*

2023-09-08 / im dong ye

viewport with auto_camera

Note:
* VpAutoCamera control setting is same as AutoCamera
  so you must edit code togather

Todo:
1. initial framebuffer size setting
2. drag imgui demo 참고해서 다시짜기
3. https://github.com/ocornut/imgui/issues/3152
	https://github.com/ocornut/imgui/issues/3492
	https://jamssoft.tistory.com/234
	https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
	https://github.com/ocornut/imgui/issues/270
	로 앱별 tag부여해서 ini 윈도우 설정 겹치지 않게

*/

#ifndef __viewport_with_camera_h_
#define __viewport_with_camera_h_

#include "../viewport.h"
#include "auto_camera.h"


namespace lim
{
	class ViewportWithCamera : public Viewport
	{
	public:
		VpAutoCamera camera;
	private:
		ViewportWithCamera(const ViewportWithCamera&) = delete;
		ViewportWithCamera& operator=(const ViewportWithCamera&) = delete;
	public:
		ViewportWithCamera(std::string_view _name, Framebuffer* createdFB)
			:Viewport(_name, createdFB), camera(this)
		{
		}
		ViewportWithCamera(ViewportWithCamera&& src) noexcept
			: Viewport(std::move(src)), camera(std::move(src.camera))
		{
		}
		virtual ~ViewportWithCamera() {}
	};
}

#endif



