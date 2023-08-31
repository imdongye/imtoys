//
//  for aplicate real-time hatching paper
//	2023-1-17 / im dong ye
//
//	TODO list:
//	
//

#ifndef __app_hatching_h_
#define __app_hatching_h_

#include <limbrary/application.h>
#include <limbrary/model_view/auto_camera.h>
#include <limbrary/program.h>
#include <limbrary/viewport.h>
#include <limbrary/model_view/model.h>
#include <glad/glad.h>

namespace lim
{
	class ArtMap: public TexBase
	{
	public:
		static constexpr GLint nr_mips = 4;
		GLint tone;
		std::string path;
	public:
		ArtMap(const std::string_view _path, GLint _tone);
	};

	class AppHatching: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "real-time hatching";
		inline static constexpr CStr APP_DIR =  "imnpr";
		inline static constexpr CStr APP_DISC = "aplicate real-time hatching paper";
	private:
		bool start_dragging = false;
		AutoCamera *camera;
		Program *program;
		Viewport *viewport;
		std::vector<Model*> models;
		Light *light;
		Model *light_model;
	private:
		static constexpr GLint nr_tones = 6;
		glm::vec2 uv_scale = {3.5f, 3.5f};
		int fixed_art_map_idx=-1;
		bool is6way = true;
		std::vector<ArtMap*> tam; // Tonal Art Map

	public:
		AppHatching();
		~AppHatching();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	};
}

#endif
