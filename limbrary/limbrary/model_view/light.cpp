/*
	2024-04-24 / im dong ye


*/
#include <limbrary/model_view/light.h>
#include <limbrary/tools/asset_lib.h>
#include <glm/gtx/transform.hpp>

using namespace glm;
using namespace lim;



ShadowMap::ShadowMap(TransformPivoted& tf) 
	: map(3, 32)
	, Enabled(true)
	, ZNear(0.f)
	, ZFar(30.f)
	, TexelSize(vec2(1/map_size))
	, OrthoSize(vec2(8,16))
	, RadiusUv(vec2(0.001f))
{
	TexelSize = glm::vec2(1.f/map_size);

	map.clear_color = glm::vec4(1);
	map.color_tex.s_wrap_param = GL_CLAMP_TO_BORDER; 
	map.color_tex.t_wrap_param = GL_CLAMP_TO_BORDER; 
	map.color_tex.border_color = glm::vec4(1.f); 
	map.resize(map_size, map_size);

	const float halfW = OrthoSize.x*0.5f;
	const float halfH = OrthoSize.y*0.5f;
	mtx_Proj = glm::ortho(-halfW, halfW, -halfH, halfH, ZNear, ZFar);
	
	tf.update_callback = [this](const Transform* tf) {
		const TransformPivoted* ptf = (const TransformPivoted*)tf;
		mtx_View = lookAt(vec3(ptf->pos), ptf->pivot, {0,1,0});
		mtx_ShadowVp = mtx_Proj * mtx_View;
	};
	tf.update();
}




LightDirectional::LightDirectional()
{
	tf.theta = 35.f;
	tf.phi =  -35.f;
	tf.dist = 7.f;
	tf.updateWithRotAndDist();
}
LightDirectional::~LightDirectional()
{
}
void LightDirectional::setShadowEnabled(bool enabled) {
	if( shadow == nullptr ) {
		shadow = new ShadowMap(tf);
	}
	shadow->Enabled = enabled;
}

void LightDirectional::bakeShadowMap(std::function<void(const glm::mat4& mtx_View, const glm::mat4& mtx_Proj)> draw) const
{
	if(!shadow || !shadow->Enabled)
		return;
	shadow->map.bind();
	
	draw(shadow->mtx_View, shadow->mtx_Proj);
	
	shadow->map.unbind();

	glBindTexture(GL_TEXTURE_2D, shadow->map.getRenderedTexId());
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void LightDirectional::setUniformTo(const Program& prog) const
{
	prog.setUniform("lit.Pos", tf.pos);
	prog.setUniform("lit.Dir", tf.dir);
	prog.setUniform("lit.Color", Color);
	prog.setUniform("lit.Intensity", Intensity);

	if( shadow == nullptr || !shadow->Enabled ) {
		prog.setUniform("shadow.Enabled", false);
	}
	else {
		prog.setUniform("mtx_ShadowVp", shadow->mtx_ShadowVp);

		prog.setUniform("shadow.Enabled", true);
		prog.setUniform("shadow.ZNear", shadow->ZNear);
		prog.setUniform("shadow.ZFar", shadow->ZFar);
		prog.setUniform("shadow.TexelSize", shadow->TexelSize );
		prog.setUniform("shadow.OrthoSize", shadow->OrthoSize );
		prog.setUniform("shadow.RadiusUv", shadow->RadiusUv );
		prog.setTexture("map_Shadow", shadow->map.getRenderedTexId());
	}
}