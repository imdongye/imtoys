/*
	2024-04-24 / im dong ye


*/
#include <limbrary/model_view/light.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/text.h>
#include <limbrary/tools/log.h>

#include <limbrary/using_in_cpp/glm.h>
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
	map.resize(ivec2(map_size));

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
















IBLight::IBLight(const char* path) {
    if( path ) {
        setMapAndBake(path);
    }
}

bool IBLight::setMapAndBake(const char* path) 
{
    if(!strIsSame(getExtension(path),"hdr")) {
        log::err("need hdr ext to load ibl\n");
        return false;
    }
    log::pure("loading ibl .. ");
    /* map_Light */
    map_Light.s_wrap_param = GL_REPEAT;
    map_Light.t_wrap_param = GL_MIRRORED_REPEAT;
    map_Light.mag_filter = GL_LINEAR;
    map_Light.min_filter = GL_LINEAR_MIPMAP_NEAREST; // Todo: mipmap 쓰면 경계 검은색 나옴;;
    if(!map_Light.initFromFile(path, false)) {
        return false;
    }



    //
    //  Bake Prefiltered Light Maps
    //
    FramebufferNoDepth fb(3, 32); // 3 channel, 32 bit
    Program iblProg("ibl baker");


    /* map_Irradiance */
    fb.color_tex.s_wrap_param = GL_REPEAT;
    fb.color_tex.t_wrap_param = GL_MIRRORED_REPEAT;
    fb.color_tex.mag_filter = GL_LINEAR;
    fb.color_tex.min_filter = GL_LINEAR; // Todo: mipmap 쓰면 경계 검은색 나옴;;
	fb.resize({256, 128});
	iblProg.attatch("canvas.vs").attatch("bake_irr.fs").link();

	fb.bind();
	iblProg.use();
	iblProg.setTexture("map_Light", map_Light.tex_id);
	AssetLib::get().screen_quad.bindAndDrawGL();
	fb.unbind();

    map_Irradiance = fb.color_tex; // !! 텍스쳐 복사되면서 mipmap도 생성됨.


    /* map_PreFilteredEnv */
    // win에서 *0.25, mac에서 /10은 해야 동작함.
    const ivec2 pfenvSize = map_Light.size/10;
    map_PreFilteredEnv.size = pfenvSize;
    map_PreFilteredEnv.nr_depth = nr_roughness_depth;
    map_PreFilteredEnv.updateFormat(3, 32);
    map_PreFilteredEnv.s_wrap_param = GL_REPEAT;
    map_PreFilteredEnv.t_wrap_param = GL_MIRRORED_REPEAT;
    map_PreFilteredEnv.r_wrap_param = GL_CLAMP_TO_EDGE;
    map_PreFilteredEnv.mag_filter = GL_LINEAR;
    map_PreFilteredEnv.min_filter = GL_LINEAR;
    map_PreFilteredEnv.initGL();

	fb.resize(pfenvSize);
    iblProg.deinitGL();
    iblProg.attatch("canvas.vs").attatch("bake_pfenv.fs").link();

    for(int i=0; i<nr_roughness_depth; i++) {
        float roughness = (i+0.5)/nr_roughness_depth;// 0.05~0.95
        fb.bind();
        iblProg.use();
        iblProg.setTexture("map_Light", map_Light.tex_id);
        iblProg.setUniform("roughness", roughness);
        AssetLib::get().screen_quad.bindAndDrawGL();
        fb.unbind();

        float* buf = fb.makeFloatPixelsBuf();
        map_PreFilteredEnv.setDataWithDepth(i, buf); // !! 텍스쳐 복사되면서 mipmap도 생성됨.
        delete buf;
    }

    /* map_PreFilteredBRDF */
    fb.color_tex.s_wrap_param = GL_CLAMP_TO_EDGE;
    fb.color_tex.t_wrap_param = GL_CLAMP_TO_EDGE;
    fb.color_tex.mag_filter = GL_LINEAR;
    fb.color_tex.min_filter = GL_LINEAR;
	fb.resize(ivec2(128));
    iblProg.deinitGL();
	iblProg.attatch("canvas.vs").attatch("bake_brdf.fs").link();

	fb.bind();
	iblProg.use();
	AssetLib::get().screen_quad.bindAndDrawGL();
	fb.unbind();

    map_PreFilteredBRDF = fb.color_tex; // !! 텍스쳐 복사되면서 mipmap도 생성됨.
    is_baked = true;
    return true;
}

void IBLight::deinitGL() {
    map_Light.deinitGL();
    map_Irradiance.deinitGL();
    map_PreFilteredEnv.deinitGL();
    map_PreFilteredBRDF.deinitGL();
}
void IBLight::setUniformTo(const Program& prg) const {
    prg.setTexture("map_Light", map_Light.tex_id);
    prg.setTexture("map_Irradiance", map_Irradiance.tex_id);
    prg.setTexture("map_PreFilteredBRDF", map_PreFilteredBRDF.tex_id);
    prg.setTexture3d("map_PreFilteredEnv", map_PreFilteredEnv.tex_id);
}