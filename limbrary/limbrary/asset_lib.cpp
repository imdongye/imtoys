#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/model_view/material.h>
#include <limbrary/log.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/model.h>

static lim::Program* env_prog;
static lim::MeshEnvSphere* env_sphere;
static int ms_max_samples = 2;

lim::AssetLib::AssetLib()
	: screen_quad(2.f, 2.f, false, false)
	, small_sphere(0.2f, 8, 4,true, false)
	, thin_cylinder(0.1f)
	, texture_viewer("replace before drawImGui", new FramebufferNoDepth(3,32))
{
	log::pure("init AssetLib\n");

	prog_tex_to_quad.name = "texToQuad";
	prog_tex_to_quad.attatch("canvas.vs").attatch("canvas_color.fs").link();
		
	prog_tex3d_to_quad.name = "tex3dToQuad";
	prog_tex3d_to_quad.attatch("canvas.vs").attatch("tex3d_to_2d.fs").link();

	prog_shadow_static.name = "shadow_skinned";
	prog_shadow_static.attatch("mvp.vs").attatch("depth.fs").link();

	prog_shadow_skinned.name = "shadow_skinned";
	prog_shadow_skinned.attatch("mvp_skinned.vs").attatch("depth.fs").link();

	prog_dnv.name = "ndv";
	prog_dnv.attatch("mvp.vs").attatch("ndv.fs").link();

	prog_color.name = "red";
	prog_color.attatch("mvp.vs").attatch("color.fs").link();

	env_prog = new Program("env");
	env_prog->attatch("mvp.vs").attatch("env.fs").link();

	env_sphere = new MeshEnvSphere(20.f);
}
lim::AssetLib::~AssetLib()
{
	delete env_prog;
	delete env_sphere;
	
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
	env_prog->use();
	env_prog->setUniform("mtx_Model", glm::mat4(1));
	env_prog->setUniform("mtx_View", mtx_View);
	env_prog->setUniform("mtx_Proj", mtx_Proj);
	env_prog->setTexture("map_Light", map.getTexId());
	env_sphere->bindAndDrawGL();
}

int lim::utils::getMsMaxSamples() {
	return ms_max_samples;
}

void lim::utils::glErr( std::string_view msg ) {
	GLint err = glGetError();
	if( err != GL_NO_ERROR ) {
		lim::log::err("GL %08x : %s\n", msg.data());
	}
}