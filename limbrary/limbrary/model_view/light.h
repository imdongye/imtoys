/*

2022-09-05 / im dong ye

point or directional light
point is pos.w == 1
direcitonal is pos.w == 0

Todo:
	roll back to no vertual func


Note:
shadow사용할때 flustum크기 시야와 오브젝트 고려해서 잘 조절해야됨.(자동으로 해주는게 필요할듯)

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
	// Todo: dinamic OrthoSize
	struct ShadowMap final : public NoCopyAndMove
	{
		inline static const int map_size = 1024;

		bool Enabled;
		float ZNear;
		float ZFar;
		glm::mat4 mtx_View;
		glm::mat4 mtx_Proj;
		glm::mat4 mtx_ShadowVp;

		FramebufferRbDepth map; // map_Shadow 
		glm::vec2 TexelSize; // ortho_width/height 고려해야하나
		glm::vec2 OrthoSize;
		glm::vec2 RadiusUv; // world space

		ShadowMap(TransformPivoted& tf);
	};

	struct LightDirectional final : public NoCopyAndMove
	{
		std::string name = "DirLight";
		TransformPivoted tf;
		glm::vec3 Color = {1,1,1};
		float Intensity = 120.f;

		OwnPtr<ShadowMap> shadow = nullptr;

		LightDirectional();
		~LightDirectional();
		void setShadowEnabled(bool enabled);
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