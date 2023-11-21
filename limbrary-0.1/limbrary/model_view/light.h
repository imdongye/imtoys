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


namespace lim
{
	class Light
	{
	public:
		// you need to find magic numbers
		int shadow_map_size = 1024;
		float shadow_z_near = 1;
		float shadow_z_far = 500;
		float ortho_width = 4;
		float ortho_height = 8;

		bool shadow_enabled = false;
		glm::vec3 color = {1,1,1};
		float intensity = 120.f;
		
		glm::vec3 pivot = {0,0,0};
		glm::vec3 position = {3.3f,5.7f,2.3f};

		FramebufferTexDepth map_Shadow;
		glm::mat4 shadow_view_mat;
		glm::mat4 shadow_proj_mat;
		glm::mat4 shadow_vp_mat;
	public:
		// theta=[0,180] up is origin
		// pi=[0,360] clockwise 3pm origin 
		// if radius<0 : maintain radius
		void setRotate(float thetaDeg=30.f, float phiDeg = 30.f, float radius = -1.f);
		void updateShadowVP();
	public:
		Light();
		Light(Light&& src) noexcept;
		Light& operator=(Light&& src) noexcept;
		~Light() noexcept;
	private:
		Light(const Light&) = delete;
		Light& operator=(const Light&) = delete;
	};
}
#endif