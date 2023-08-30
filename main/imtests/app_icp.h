//
//  icp test app
//	2022-11-14 / im dong ye
//

#ifndef __app_icp_h_
#define __app_icp_h_

#include <limbrary/application.h>
#include <limbrary/model_view/auto_camera.h>
#include <limbrary/program.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppICP: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "icp test app";
		inline static constexpr CStr APP_DIR  = "imtests";
		inline static constexpr CStr APP_DISC = "icp is iterative closest point";
	private:
		AutoCamera* camera;
		Program* prog;
		Model* model;
		Mesh *src, *dest;
		glm::mat4 icp_mat = glm::mat4(1);
	public:
		AppICP();
		~AppICP();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	};
}

#endif
