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
#include <limbrary/model_view/light.h>
#include <glm/gtx/transform.hpp>

using namespace glm;

namespace lim
{
	Light::Light()
	{
		map_Shadow.clear_color = glm::vec4(1);
		map_Shadow.resize(shadow_map_size, shadow_map_size);

		// fov 1.0은 60도 정도 2에서 1~-1사이의 중앙모델만 그린다고 가정하면 far을 엄청 멀리까지 안잡아도되고
		// depth의 4바이트 깊이를 많이 사용할수있다.
		// proj_mat = glm::perspective(1.f, 1.f, shadow_z_near, shadow_z_far);

		const float halfW = ortho_width*0.5f;
		const float halfH = ortho_height*0.5f;
		shadow_proj_mat = glm::ortho(-halfW, halfW, -halfH, halfH, shadow_z_near, shadow_z_far);
		updateWithPivotAndPos();
	}
	Light::Light(Light&& src) noexcept
	{
		*this = std::move(src);
	}
	Light& Light::operator=(Light&& src) noexcept
	{
		if( this==&src )
			return *this;
		map_Shadow = std::move(src.map_Shadow);
		shadow_map_size = src.shadow_map_size;
		shadow_z_near = src.shadow_z_near;
		shadow_z_far = src.shadow_z_far;
		ortho_width = src.ortho_width;
		ortho_height = src.ortho_height;
		shadow_enabled = src.shadow_enabled;
		color = src.color;
		intensity = src.intensity;
		pivot = src.pivot;
		position = src.position;
		shadow_view_mat = src.shadow_view_mat;
		shadow_proj_mat = src.shadow_proj_mat;
		shadow_vp_mat = src.shadow_vp_mat;
		return *this;
	}
	Light::~Light() noexcept
	{
	}
	void Light::setRotate(float thetaDeg, float phiDeg, float radius)
	{
		vec3 diff = position-pivot;
		float distance = (radius<0)?length(diff):radius;

		vec4 toLit = {0,1,0,0};
		toLit = rotate(radians(thetaDeg), vec3{0,0,1}) * toLit;
		toLit = rotate(radians(phiDeg), vec3{0,1,0}) * toLit;

		position = pivot+distance*vec3(toLit);
		updateWithPivotAndPos();
	}
	void Light::updateWithPivotAndPos()
	{
		shadow_view_mat = lookAt(vec3(position), pivot, {0,1,0});
		shadow_vp_mat = shadow_proj_mat * shadow_view_mat;
	}

}
