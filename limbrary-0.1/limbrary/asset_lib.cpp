#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/model_view/material.h>
#include <limbrary/log.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/model.h>

static lim::Program* env_prog;
static lim::Model* env_sphere;
static int ms_max_samples = 2;

lim::AssetLib::AssetLib()
	: screen_quad(false, false)
	, small_sphere(8,4,true, false, 0.2f)
{
	log::pure("init AssetLib\n");

	tex_to_quad_prog.name = "texToQuad";
	tex_to_quad_prog.attatch("canvas.vs").attatch("canvas_color.fs").link();
		
	tex3d_to_quad_prog.name = "tex3dToQuad";
	tex3d_to_quad_prog.attatch("canvas.vs").attatch("tex3d_to_2d.fs").link();

	depth_prog.name = "depth";
	depth_prog.attatch("mvp.vs").attatch("depth.fs").link();

	ndv_prog.name = "ndv";
	ndv_prog.attatch("mvp.vs").attatch("ndv.fs").link();

	default_prog.name = "defualt";
	default_prog.attatch("mvp.vs").attatch("ndv.fs").link();
	default_material.prog = &default_prog;
	default_material.set_prog = [](const Program&){};



	env_prog = new Program("env");
	env_prog->attatch("mvp.vs").attatch("env.fs").link();

	env_sphere = new Model("env");
	env_sphere->my_meshes.push_back(new MeshEnvSphere());
	env_sphere->tf->scale = glm::vec3(20.f);
	env_sphere->tf->update();


	glGetIntegerv(GL_MAX_SAMPLES, &ms_max_samples);
	log::pure("GL_MAX_SAMPLES : %d\n", ms_max_samples);
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


void lim::utils::drawEnvSphere(const Texture& map, const glm::mat4& viewMat, const glm::mat4& projMat) {
	env_prog->use();
	env_prog->setUniform("model_Mat", env_sphere->tf->mat);
	env_prog->setUniform("view_Mat", viewMat);
	env_prog->setUniform("proj_Mat", projMat);
	env_prog->setTexture("map_Light", map.getTexId(), 0);
	env_sphere->my_meshes.back()->drawGL();
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