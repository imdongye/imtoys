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
	: screen_quad(false, false)
	, small_sphere( 0.2f, 8,4,true, false)
	, thin_cylinder(0.1f)
	, texture_viewer("replace before drawImGui", new FramebufferNoDepth(3,32))
{
	log::pure("init AssetLib\n");

	tex_to_quad_prog.name = "texToQuad";
	tex_to_quad_prog.attatch("canvas.vs").attatch("canvas_color.fs").link();
		
	tex3d_to_quad_prog.name = "tex3dToQuad";
	tex3d_to_quad_prog.attatch("canvas.vs").attatch("tex3d_to_2d.fs").link();

	depth_prog.name = "depth";
	depth_prog.attatch("mvp_no_out.vs").attatch("depth.fs").link();

	ndv_prog.name = "ndv";
	ndv_prog.attatch("mvp.vs").attatch("ndv.fs").link();

	color_prog.name = "red";
	color_prog.attatch("mvp.vs").attatch("color.fs").link();

	skin_xfb_prog.name = "skinning_xfb";
	skin_xfb_prog.attatch("skin_xfb.vs").link();
	glGenTransformFeedbacks(1, &skin_xfb_id);

	env_prog = new Program("env");
	env_prog->attatch("mvp.vs").attatch("env.fs").link();

	env_sphere = new MeshEnvSphere(20.f);


	glGetIntegerv(GL_MAX_SAMPLES, &ms_max_samples);
	log::pure("GL_MAX_SAMPLES : %d\n\n", ms_max_samples);
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