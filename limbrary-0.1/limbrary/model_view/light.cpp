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
#include <limbrary/framebuffer.h>
#include <glm/gtx/transform.hpp>
#include <limbrary/asset_lib.h>
#include <limbrary/model_view/model.h>


namespace lim
{
	Light::Light(float _yaw, float _pitch, glm::vec3 _color, float _intensity, bool shadowEnabled)
	{
		yaw=_yaw; pitch=_pitch; color=_color;
		intensity=_intensity; shadow_enabled=shadowEnabled;

		depth_prog = AssetLib::get().depth_prog;

		shadow_map.clear_color = glm::vec4(1);
		shadow_map.resize(shadow_map_size, shadow_map_size);

		// fov 1.0은 60도 정도 2에서 1~-1사이의 중앙모델만 그린다고 가정하면 far을 엄청 멀리까지 안잡아도되고
		// depth의 4바이트 깊이를 많이 사용할수있다.
		// proj_mat = glm::perspective(1.f, 1.f, shadow_z_near, shadow_z_far);

		const float halfW = ortho_width*0.5f;
		const float halfH = ortho_height*0.5f;
		proj_mat = glm::ortho(-halfW, halfW, -halfH, halfH, shadow_z_near, shadow_z_far);
		updateMembers();
	}
	Light::~Light()
	{
	}
	void Light::updateMembers()
	{
		yaw = glm::mod(yaw, 360.f);
		//const float minPitchDegree = 20;
		pitch = glm::clamp(pitch, 20.f, 89.f);
		position = glm::vec3(glm::rotate(glm::radians(yaw), glm::vec3(0, 1, 0))
								* glm::rotate(glm::radians(-pitch), glm::vec3(1, 0, 0))
								* glm::vec4(0, 0, distance, 1));
		direction = glm::normalize(position);

		view_mat = glm::lookAt(position, {0,0,0}, {0,1,0});
		vp_mat = proj_mat * view_mat;
	}
	void Light::drawShadowMap(std::vector<Model*> models) const
	{
		shadow_map.bind();

		depth_prog->use();
		depth_prog->setUniform("viewMat", view_mat);
		depth_prog->setUniform("projMat", proj_mat);

		for( Model* md : models ) {
			depth_prog->setUniform("modelMat", md->model_mat);
			for( Mesh* ms : md->meshes ) {
				glBindVertexArray(ms->VAO);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms->EBO);
				glDrawElements(ms->draw_mode, static_cast<GLuint>(ms->indices.size()), GL_UNSIGNED_INT, 0);
			}
		}

		shadow_map.unbind();
	}
}
