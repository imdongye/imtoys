/*
	imdongye@naver.com
	fst: 2022-09-05
	lst: 2022-09-05

Note:
	shadow사용할때 flustum크기 시야와 오브젝트 고려해서 잘 조절해야됨.(자동으로 해주는게 필요할듯)
	point or directional light
	point is pos.w == 1
	direcitonal is pos.w == 0

Todo:
	roll back to no vertual func

*/

#ifndef __light_h_
#define __light_h_

#include "../framebuffer.h"
#include <glm/glm.hpp>
#include <functional>
#include "../program.h"
#include "transform.h"
#include "../tools/mecro.h"
#include "../containers/own_ptr.h"


namespace lim
{
	struct ShadowMap final : public NoCopyAndMove
	{
		//
		// vert shader
		//
		glm::mat4 mtx_View;
		glm::mat4 mtx_Proj;
		glm::mat4 mtx_ShadowVp;

		//
		// frag shader
		//
		bool Enabled;
		float Bias;
		float ZNear;
		float ZFar;

		bool IsOrtho;
		glm::vec2 OrthoSize;
		float Fov; // (degree) if Fov is 0 then use ortho or perspective
		float Aspect;
		
		glm::vec2 TexelSize; // menualy depend on tex_size (currently not used)
		bool UsePCSS;
		glm::vec2 RadiusUv; // if UsePCSS area radius or PCF radius. and world space unit

		//
		// application
		//
		FramebufferRbDepth map; // map_Shadow 
		int tex_size;


		ShadowMap(TransformPivoted& tf);
		void applyMapSize();
		void applyProjMtx();
	};

	struct LightDirectional final : public NoCopyAndMove
	{
		std::string name = "DirLight";
		TransformPivoted tf;
		glm::vec3 Color = {1,1,1};
		float Intensity = 120.f;

		OwnPtr<ShadowMap> shadow = nullptr;

		LightDirectional(bool withShadow = false);
		~LightDirectional();
		void bakeShadowMap(std::function<void(const glm::mat4& mtx_View, const glm::mat4& mtx_Proj)> draw) const;
		void setUniformTo(const Program& prog) const;
	};




	struct LightSpot final : public NoCopyAndMove
	{
		std::string name = "DirLight";
		TransformPivoted tf;
	};




	struct ShadowCubeMap final : public NoCopyAndMove
	{

	};

	struct LightOmni final : public NoCopyAndMove
	{
		std::string name = "OmniLight";
		Transform tf;

		OwnPtr<ShadowCubeMap> shadow = nullptr;
	};


	// pre-filtered, split-sum
	struct IBLight final : public NoCopyAndMove
	{
		static constexpr int nr_roughness_depth = 10;
		std::string name = "ImageBasedLight";
		Texture map_Light; // env map
        Texture map_Irradiance;
        Texture map_PreFilteredBRDF;
        Texture3d map_PreFilteredEnv;
        bool is_baked = false;

		IBLight(const char* path = nullptr);
        ~IBLight() = default;

        bool setMapAndBake(const char* path);
        void deinitGL();
        void setUniformTo(const Program& prg) const;
	};
}
#endif