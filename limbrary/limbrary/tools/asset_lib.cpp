#include <limbrary/tools/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/model_view/material.h>
#include <limbrary/tools/log.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/model.h>


lim::AssetLib::AssetLib()
	: screen_quad(2.f, 2.f, false, false)
	, sphere(1.f, 50, 25, true, true)
	, small_sphere(0.2f, 8, 4,true, false)
	, thin_cylinder(0.1f)
	, texture_viewer("replace before drawImGui", new FramebufferNoDepth(3,32))
	, env_sphere(20.f)
{
	screen_quad.initGL(true);
	sphere.initGL(true);
	small_sphere.initGL(true);
	thin_cylinder.initGL(true);
	env_sphere.initGL(true);


	log::pure("init AssetLib\n");

	prog_tex_to_quad.name = "texToQuad";
	prog_tex_to_quad.attatch("canvas.vs").attatch("canvas_color.fs").link();
		
	prog_tex3d_to_quad.name = "tex3dToQuad";
	prog_tex3d_to_quad.attatch("canvas.vs").attatch("tex3d_to_2d.fs").link();

	prog_shadow_static.name = "shadow_skinned";
	prog_shadow_static.attatch("mvp.vs").attatch("depth.fs").link();

	prog_shadow_skinned.name = "shadow_skinned";
	prog_shadow_skinned.attatch("mvp_skinned.vs").attatch("depth.fs").link();

	prog_ndv.name = "ndv";
	prog_ndv.attatch("mvp.vs").attatch("ndv.fs").link();

	prog_color.name = "red";
	prog_color.attatch("mvp.vs").attatch("color.fs").link();

	prog_env.name = "env";
	prog_env.attatch("mvp.vs").attatch("env.fs").link();
}

lim::AssetLib::~AssetLib()
{
	log::pure("delete AssetLib\n");
}
void lim::AssetLib::create(AppBase* _app)
{
	if( !instance )
		instance = new AssetLib();
	instance->app = _app;
}
lim::AssetLib& lim::AssetLib::get()
{
	return *instance;
}
void lim::AssetLib::destroy()
{
	if( instance )
		delete instance;
	instance = nullptr;
}






void lim::utils::drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj) {
	const Program& prog = AssetLib::get().prog_env.use();
	prog.setUniform("mtx_Model", glm::mat4(1));
	prog.setUniform("mtx_View", mtx_View);
	prog.setUniform("mtx_Proj", mtx_Proj);
	prog.setTexture("map_Light", map.getTexId());
	AssetLib::get().env_sphere.bindAndDrawGL();
}

static int ms_max_samples = 2;

int lim::utils::getMsMaxSamples() {
	return ms_max_samples;
}

void lim::utils::glErr( std::string_view msg ) {
	GLint err = glGetError();
	if( err != GL_NO_ERROR ) {
		lim::log::err("GL %08x : %s\n", msg.data());
	}
}