#include <limbrary/tools/asset_lib.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/text.h>
#include <limbrary/application.h>

#include <iomanip>


using namespace lim;

// var definition
const Mesh* asset_lib::screen_quad;
const Mesh* asset_lib::big_plane;
const Mesh* asset_lib::sphere;
const Mesh* asset_lib::small_sphere;
const Mesh* asset_lib::thin_cylinder;
const Mesh* asset_lib::env_sphere;

const Program* asset_lib::prog_tex_to_quad;
const Program* asset_lib::prog_tex3d_to_quad;
const Program* asset_lib::prog_shadow_static;
const Program* asset_lib::prog_shadow_skinned;
const Program* asset_lib::prog_ndv;
const Program* asset_lib::prog_color;
const Program* asset_lib::prog_env;

Viewport* asset_lib::texture_viewer;



void asset_lib::init()
{
	log::pure("asset_lib: created\n");

	Mesh* tms;
	tms = new MeshQuad(2.f, 2.f, false, false); 		tms->initGL(); screen_quad = tms;
	tms = new MeshPlane(20.f, 20.f, 1, 1, true, true); 	tms->initGL(); big_plane = tms;
	tms = new MeshSphere(1.f, 50, 25, true, true); 		tms->initGL(); sphere = tms;
	tms = new MeshSphere(0.2f, 8, 4,true, false); 		tms->initGL(); small_sphere = tms;
	tms = new MeshCylinder(0.1f); 						tms->initGL(); thin_cylinder = tms; 
	tms = new MeshEnvSphere(80.f); 						tms->initGL(); env_sphere = tms;


	Program* tpg;

	tpg = new Program("texToQuad"); tpg->attatch("canvas.vs").attatch("tex_to_2d.fs").link(); prog_tex_to_quad = tpg;
	tpg = new Program("tex3dToQuad"); tpg->attatch("canvas.vs").attatch("tex3d_to_2d.fs").link(); prog_tex3d_to_quad = tpg;
	tpg = new Program("shadow_static"); tpg->attatch("mvp.vs").attatch("depth.fs").link(); prog_shadow_static = tpg;
	tpg = new Program("shadow_skinned"); tpg->attatch("mvp_skinned.vs").attatch("depth.fs").link(); prog_shadow_skinned = tpg;
	tpg = new Program("ndv"); tpg->attatch("mvp.vs").attatch("ndv.fs").link(); prog_ndv = tpg;
	tpg = new Program("color"); tpg->attatch("mvp.vs").attatch("color.fs").link(); prog_color = tpg;
	tpg = new Program("env"); tpg->attatch("mvp.vs").attatch("env.fs").link(); prog_env = tpg;
	
	texture_viewer = new Viewport(new FramebufferNoDepth(3,32), fmtStrToBuf("TextureViewer##%s", AppBase::g_app_name));
}

void asset_lib::deinit()
{
	delete screen_quad;
	delete big_plane;
	delete sphere;
	delete small_sphere;
	delete thin_cylinder;
	delete env_sphere;

	delete prog_tex_to_quad;
	delete prog_tex3d_to_quad;
	delete prog_shadow_static;
	delete prog_shadow_skinned;
	delete prog_ndv;
	delete prog_color;
	delete prog_env;

	delete texture_viewer;
}