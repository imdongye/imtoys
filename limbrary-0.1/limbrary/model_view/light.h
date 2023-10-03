/*

2022-09-05 / im dong ye

point or directional light
point is pos.w == 1
direcitonal is pos.w == 0

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
		int shadow_map_size = 1024;
		float shadow_z_near = 3;
		float shadow_z_far = 6;
		float ortho_width = 4;
		float ortho_height = 8;


		bool shadow_enabled = false;
		glm::vec3 color = {1,1,1};
		float intensity = 1.f;
		
		glm::vec3 pivot = {0,0,0};
		glm::vec3 position = {50,50,50};

		TexFramebuffer map_Shadow;
		glm::mat4 shadow_view_mat;
		glm::mat4 shadow_proj_mat;
		glm::mat4 shadow_vp_mat;
	private:
		Light(const Light&) = delete;
		Light& operator=(const Light&) = delete;
	public:
		Light();
		Light(Light&& src) noexcept;
		Light& operator=(Light&& src) noexcept;
		~Light() noexcept;
		// theta=[0,180] up is origin
		// pi=[0,360] clockwise 3pm origin 
		// if radius<0 : maintain radius
		void setRotate(float thetaDeg=30.f, float phiDeg = 30.f, float radius = -1.f);
		void updateWithPivotAndPos();
	};
}
#endif