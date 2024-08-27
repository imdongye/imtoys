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
#include "../containers/own_ptr.h"


namespace lim
{
	// Todo: dinamic OrthoSize
	struct ShadowMap
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

	struct LightDirectional
	{
		TransformPivoted tf;
		glm::vec3 Color = {1,1,1};
		float Intensity = 120.f;

		OwnPtr<ShadowMap> shadow = nullptr;

		LightDirectional();
		LightDirectional(const LightDirectional& src) = delete;
		LightDirectional& operator=(const LightDirectional& src) = delete;
		LightDirectional(LightDirectional&& src) = delete;
		LightDirectional& operator=(LightDirectional&& src) = delete;
		~LightDirectional();
		void setShadowEnabled(bool enabled);
		void bakeShadowMap(std::function<void(const glm::mat4& mtx_View, const glm::mat4& mtx_Proj)> draw) const;
		void setUniformTo(const Program& prog) const;
	};




	struct LightSpot
	{
		TransformPivoted tf;
	};




	struct ShadowCubeMap
	{

	};

	struct LightOmni
	{
		OwnPtr<ShadowMap> shadow = nullptr;
		Transform tf;
	};
}
#endif