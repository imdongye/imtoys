#ifndef __app_gen_mesh_h_
#define __app_gen_mesh_h_

#include <limbrary/application.h>
#include <limbrary/model_view/auto_camera.h>
#include <limbrary/program.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppGenMesh : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "code mesh viewer";
		inline static constexpr CStr APP_DIR  = "imtests";
		inline static constexpr CStr APP_DISC = "test my code meshes";

	private:
		// ui vars
		glm::vec2 uv_scale = {1.f, 1.f};
		float vs_t = 0.f;

		AutoCamera* camera;

		Program* program;
		Viewport* viewport;
		std::vector<Model*> models;
		Light* light;
		Model* light_model;
		TexBase* debugging_tex;

	public:
		AppGenMesh();
		~AppGenMesh();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	private:
		void processInput(GLFWwindow* window);
	};
}

#endif
