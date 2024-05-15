/*
	minesweeper
	2024-05-14 / imdongye
*/

#ifndef __app_mines_weeper_h_
#define __app_mines_weeper_h_

#include <limbrary/application.h>

namespace lim
{
	class AppMineSweeper : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "minesweeper demo";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DESCRIPTION = "...";
	public:
		AppMineSweeper();
		~AppMineSweeper();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
