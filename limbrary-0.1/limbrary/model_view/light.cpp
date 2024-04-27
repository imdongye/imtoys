/*
	2024-04-24 / im dong ye


*/
#include <limbrary/model_view/light.h>
#include <limbrary/asset_lib.h>
#include <glm/gtx/transform.hpp>

using namespace glm;
using namespace lim;

ILight::ILight(enum LightType lt) : light_type((int)lt)
{
	tf.rot = {35.f, -35.f};
	tf.distance = 7.f;
	tf.updatePosDir();
}



LightDirectional::Shadow::Shadow(const TransformPivoted* _tf, const glm::vec2* lightRadiusUv) 
	: map(3, 32), tf(_tf), radius_wuv(lightRadiusUv)
{
	map.clear_color = glm::vec4(1);
	map.color_tex.s_wrap_param = GL_CLAMP_TO_BORDER; 
	map.color_tex.t_wrap_param = GL_CLAMP_TO_BORDER; 
	map.color_tex.border_color = glm::vec4(1.f); 
	map.resize(map_size, map_size);

	const float halfW = ortho_width*0.5f;
	const float halfH = ortho_height*0.5f;
	proj_mat = glm::ortho(-halfW, halfW, -halfH, halfH, z_near, z_far);
	updateVP();
	updateRadiusTexSpaceUv();
}
void LightDirectional::Shadow::updateVP() {
	view_mat = lookAt(vec3(tf->position), tf->pivot, {0,1,0});
	vp_mat = proj_mat * view_mat;
}
void LightDirectional::Shadow::updateRadiusTexSpaceUv() {
	const float coef = 1.f/200.f;
	radius_tuv = coef * (*radius_wuv) / glm::vec2(ortho_width, ortho_height);
}




LightDirectional::LightDirectional() : ILight(ILight::LT_DIRECTIONAL)
{
}
LightDirectional::~LightDirectional()
{
	if(shadow) {
		delete shadow;
		shadow = nullptr;
	}
}
void LightDirectional::setShadowEnabled(bool enabled) {
	if( shadow == nullptr ) {
		shadow = new Shadow(&tf, &shadow_radius_uv);
	}
	shadow->enabled = enabled;
}

void LightDirectional::bakeShadowMap(const std::vector<const Model*>& mds) 
{
	if(!shadow || !shadow->enabled)
		return;
	const Program& depthProg = AssetLib::get().depth_prog;

	shadow->map.bind();
	depthProg.setUniform("view_Mat", shadow->view_mat);
	depthProg.setUniform("proj_Mat", shadow->proj_mat);
            
	for( const Model* pMd : mds ) {
		pMd->root.treversal([&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
			depthProg.setUniform("model_Mat", transform);
			ms->drawGL();
		});
	}

	shadow->map.unbind();

	glBindTexture(GL_TEXTURE_2D, shadow->map.getRenderedTexId());
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void LightDirectional::setUniformTo(const Program& prog) 
{
	prog.setUniform("light_Type", light_type);
	prog.setUniform("light_Dir", tf.direction);
	prog.setUniform("light_Color", color);
	prog.setUniform("light_Int", intensity);

	if( shadow == nullptr || !shadow->enabled ) {
		prog.setUniform("shadow_Enabled", false);
	}
	else {
		prog.setUniform("shadow_vp_Mat", shadow->vp_mat);

		prog.setUniform("shadow_Enabled", true);
		prog.setUniform("shadow_z_Near", shadow->z_near);
		prog.setUniform("shadow_z_Far", shadow->z_far);
		prog.setUniform("shadow_radius_Uv", shadow->radius_tuv );
		prog.setTexture("map_Shadow", shadow->map.getRenderedTexId());
	}
}