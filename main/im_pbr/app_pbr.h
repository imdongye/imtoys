//
//  for test pbr
//	edit learnopengl code 
//	2022-11-28 / im dong ye
//
//	TODO list:
//	1. render sphere isVertexArray가 무거운지 확인, vbo, gpu 버퍼가 언제 clear되는지 질문
//

#ifndef __app_pbr_h_
#define __app_pbr_h_

#include <limbrary/application.h>
#include <limbrary/model_view/viewport_with_camera.h>
#include <limbrary/program.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppPbr: public AppBase
	{
	public:
        inline static constexpr CStr APP_NAME = "pbr tester";
		inline static constexpr CStr APP_DIR  = "im_pbr";
		inline static constexpr CStr APP_DISC = "ggx beckman";
	private:
		Program* prog;
        ViewportWithCamera* viewport;
		Mesh* sphere;
		Model* model;

		int nr_rows    = 4;
		int nr_cols = 4;
		float spacing = 2.5;

		/* lights */
		bool movedLight = false;
		glm::vec3 light_position = glm::vec3(-10.0f, 10.0f, 10.0f);
		glm::vec3 light_color = glm::vec3(300.0f, 300.0f, 300.0f);

		glm::vec3 albedo = glm::vec3(0.5, 0.f, 0.f);
		bool start_dragging = false;
		std::vector<glm::vec3> metal_colors;
		float roughness = 0.3f;
		float metallic = 1.0f;
		float beckmannGamma = 1.265f;

	public:
		AppPbr();
		~AppPbr();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	};
}

#endif
