#ifndef __app_gen_mesh_h_
#define __app_gen_mesh_h_

#include <limbrary/application.h>
#include <limbrary/program.h>
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/renderer.h>

namespace lim
{
	class AppGenMesh : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "code mesh viewer";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DESCRIPTION = "test my code meshes";

	private:
		// ui vars
		glm::vec2 uv_scale = {1.f, 1.f};
		float vs_t = 0.f;

		Program* program;
		ViewportWithCamera* viewport;
		std::vector<Model*> models;
		Light* light;
		Model* light_model;
		Texture* debugging_tex;
		Scene scene;

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
