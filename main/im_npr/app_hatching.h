//
//  for aplicate real-time hatching paper
//	2023-1-17 / im dong ye
//
//	Todo:
//	
//

#ifndef __app_hatching_h_
#define __app_hatching_h_

#include <limbrary/application.h>
#include <limbrary/program.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/model.h>
#include <glad/glad.h>
#include <limbrary/3d/scene.h>

namespace lim
{
	class ArtMap: public Texture
	{
	public:
		static constexpr GLint nr_mips = 4;
		GLint tone;
		std::string path;
	public:
		ArtMap(const char* _path, GLint _tone);
	};

	class AppHatching: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "real-time hatching";
		inline static constexpr CStr APP_DIR  = "im_npr/";
		inline static constexpr CStr APP_INFO = "aplicate real-time hatching paper";
	private:
		bool start_dragging = false;
		
		Material h_mat;
		Program h_prog;

		ViewportWithCam viewport;
		Scene scene;

		
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
		virtual void updateImGui() override;
	};
}

#endif
