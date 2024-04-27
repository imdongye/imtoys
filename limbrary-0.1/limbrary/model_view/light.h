/*

2022-09-05 / im dong ye

point or directional light
point is pos.w == 1
direcitonal is pos.w == 0

Note:
shadow사용할때 flustum크기 시야와 오브젝트 고려해서 잘 조절해야됨.(자동으로 해주는게 필요할듯)

*/

#ifndef __light_h_
#define __light_h_

#include "../framebuffer.h"
#include <glm/glm.hpp>
#include "../program.h"
#include "model.h"


namespace lim
{
	class ILight
	{
	public:
		enum LightType {
			LT_DIRECTIONAL,
			LT_POINT,
			LT_SPOT,
		};
		int light_type;

		TransformPivoted tf;
		glm::vec3 color = {1,1,1};
		float intensity = 120.f;

	protected:
		ILight(const ILight&)	         = delete;
		ILight(ILight&&)			     = delete;
		ILight& operator=(const ILight&) = delete;
		ILight& operator=(ILight&&)      = delete;

		ILight(enum LightType lt);
		virtual ~ILight() = default;

	public:
		virtual void setShadowEnabled(bool enabled) = 0;
		virtual void bakeShadowMap(const std::vector<const Model*>& mds) = 0;
		virtual void setUniformTo(const Program& prog) = 0;
	};




	class LightDirectional : public ILight
	{
	public:
		struct Shadow {
			bool enabled = true;
			int map_size = 1024;
			float z_near = 0.0f;
			float z_far = 30.f;
			float ortho_width = 4;
			float ortho_height = 8;
			FramebufferRbDepth map;

			const glm::vec2* radius_wuv; // world space
			glm::vec2 radius_tuv; // texture space

			const TransformPivoted* tf;
			glm::mat4 view_mat;
			glm::mat4 proj_mat;
			glm::mat4 vp_mat;

			Shadow(const TransformPivoted* _tf, const glm::vec2* lightRadiusUv);
			void updateVP();
			void updateRadiusTexSpaceUv();
		};

		Shadow* shadow = nullptr;
		glm::vec2 shadow_radius_uv = {1,1};
		glm::vec3 direction;

	public:
		LightDirectional();
		~LightDirectional();
		virtual void setShadowEnabled(bool enabled) override;
		virtual void bakeShadowMap(const std::vector<const Model*>& mds) override;
		virtual void setUniformTo(const Program& prog) override;
	};




	
	// class LightPoint : public ILight
	// {
	// };




	// class LightSpot : public ILight
	// {
	// };
}
#endif