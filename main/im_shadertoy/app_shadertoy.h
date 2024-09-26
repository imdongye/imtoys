/*

offline shadertoy
2022-11-14 / im dong ye

*/

#ifndef __app_shadertoy_h_
#define __app_shadertoy_h_

#include <limbrary/application.h>
#include <limbrary/viewport.h>
#include <limbrary/program.h>
#include <limbrary/texture.h>
#include <limbrary/model_view/mesh_maked.h>

namespace lim
{
	class ProgramShaderToy : public ProgramReloadable
	{
	private:
		std::string glsl_path;
		void makeMergedFragmentShader();
	public:
		ProgramShaderToy(const char* glslPath="im_shadertoy/shaders/debug.glsl");
		void reload(const char* glslPath = nullptr);
	};

	class AppShaderToy : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "ImShaderToys";
		inline static constexpr CStr APP_DIR  = "im_shadertoy/";
		inline static constexpr CStr APP_INFO = "offline shadertoy";
	private:
		Viewport viewport;
		ProgramShaderToy program;
		Texture iChannel0;
	public:
		AppShaderToy();
		~AppShaderToy();
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
