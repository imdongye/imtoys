#ifndef __app_gen_mesh_h_
#define __app_gen_mesh_h_

#include <limbrary/application.h>
#include <limbrary/program.h>
#include <limbrary/3d/model.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/scene.h>

namespace lim
{
	class AppGenMesh : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "code mesh viewer";
		inline static constexpr CStr APP_DIR  = "im_tests/";
		inline static constexpr CStr APP_INFO = "test my code meshes";

	private:
		// ui vars
		glm::vec2 uv_scale = {1.f, 1.f};
		float vs_t = 0.f;

		Program program;
		Material default_mat;
		ViewportWithCam viewport;
		Texture debugging_tex;
		Scene scene;

	public:
		AppGenMesh();
		~AppGenMesh();
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	private:
		void addMeshToScene(Mesh* ms);
	};
}

#endif
