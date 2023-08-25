//
//  framework template
//	2022-11-14 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef __app_icp_h_
#define __app_icp_h_

#include <limbrary/application.h>
#include <limbrary/model_view/auto_camera.h>
#include <limbrary/program.h>
#include <limbrary/viewport.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppICP: public AppBase
	{
	public:
		inline static constexpr const char *const APP_NAME = "icp test app";
		inline static constexpr const char *const APP_DIR  = "imtests/";
		inline static constexpr const char *const APP_DISC = "icp is iterative closest point";
	private:
		AutoCamera* camera;
		Program* prog;
		Viewport* viewport;
		Model* model;
		Mesh *src, *dest;
		glm::mat4 icpMat = glm::mat4(1);
	public:
		AppICP();
		~AppICP();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	};
}

#endif
