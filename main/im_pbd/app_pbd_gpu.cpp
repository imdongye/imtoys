#include "app_pbd_gpu.h"
#include <limbrary/limgui.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace glm;

namespace {
	AppPbdGpu* g_app = nullptr;
	pbd::SoftBodyGpu* body = nullptr;
	pbd::SimulatorGpu simulator;
	int nr_cloth_width = 4;
	bool is_paused = true;
}

static void resetApp() {
	pbd::SoftBody::Compliance tempComp;
	if(body) {
		tempComp = body->compliance;
		delete body;
	}

	{
		int nrShear = 2;
		float mass = 1.f;
		auto bendType = pbd::SoftBody::BendType::None;
		MeshPlane ms(1.f, nr_cloth_width, nr_cloth_width, false, false);
		mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
		// mat4 tf = translate(vec3(0,2,0));
		for( vec3& p : ms.poss ) {
			p = vec3(tf*vec4(p,1));
		}
		body = new pbd::SoftBodyGpu(ms, nrShear, bendType, mass);
		body->w_s[0] = 0.f;
		body->w_s[nr_cloth_width] = 0.f;
		body->compliance = tempComp;
	}

	simulator.body = body;
}


AppPbdGpu::AppPbdGpu() : AppBaseCanvas3d(1200, 780, APP_NAME, false, 10, 100, 100)
{
	prog_ms.attatch("mvp.vs").attatch("ndl.fs").link();
	g_app = this;
	resetApp();
}
AppPbdGpu::~AppPbdGpu()
{
}

void AppPbdGpu::canvasUpdate()
{
	if( is_paused )
		return;
	simulator.update(delta_time);
}


void AppPbdGpu::customDrawShadow(const mat4& mtx_View, const mat4& mtx_Proj) const
{
	Program& sProg = lim::AssetLib::get().prog_shadow_static;
	sProg.use();
	sProg.setUniform("mtx_View", mtx_View);
	sProg.setUniform("mtx_Proj", mtx_Proj);
	sProg.setUniform("mtx_Model", glm::mat4(1));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void AppPbdGpu::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void AppPbdGpu::canvasDraw() const
{
	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawQuad(vec3{0}, {0,1,0}, {0,0.3f,0});
}

void AppPbdGpu::canvasImGui()
{
}