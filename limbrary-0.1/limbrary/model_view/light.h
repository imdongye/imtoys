//
//  2022-09-05 / im dong ye
// 
//	position에서 vec3(0)(중앙바닥)을 바라보는 direction light
// 
//	point light를 만든다면
//  draw 함수포인터에서 light의 위치를 받아서 모델자신을 바라보는 viewMat을 계산해서 적용해줘야하나?
//	그럼 이때 알맞은 fov는 뭐지?
//
//	only one light
//

#ifndef __light_h_
#define __light_h_

#include "../program.h"
#include "../framebuffer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <functional>


namespace lim
{
	class Light // direction
	{
	public:
		Program* depth_prog;

		const GLuint shadow_map_size = 1024;
		const float shadow_z_near = 3;
		const float shadow_z_far = 6;
		const float ortho_width = 4;
		const float ortho_height = 8;
		float distance = 5;
	public: // editable
		bool shadow_enabled; // 밖에서 확인하고 사용하기
		float yaw, pitch;
		glm::vec3 color;
		float intensity;
	public: // update result
		glm::vec3 position;
		glm::vec3 direction;
		TexFramebuffer shadow_map;
		glm::mat4 view_mat;
		glm::mat4 proj_mat;
		glm::mat4 vp_mat;
	public:
		Light(float _yaw=58.f, float _pitch=51.f, glm::vec3 _color={1,1,1}, float _intensity = 1.f, bool shadowEnabled=false);
		~Light();
	public:
		void updateMembers();
		void setUniforms(const Program& prog) const;
		/* light 와 model의 circular dependency때문에 shadowMap을 직접 그리는 함수를 외부에서 정의하게함. */
		// todo: solve circular dependency with inl file
		void drawShadowMap(std::vector<Model*> models);
	};
}
#endif