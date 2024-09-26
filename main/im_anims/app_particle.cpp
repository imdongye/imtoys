#include "app_particle.h"
#include <limbrary/program.h>
#include <vector>
#include <limbrary/tools/limgui.h>

using namespace std;
using namespace lim;
using namespace glm;

namespace {
	constexpr int nr_invocations = 1000;
	constexpr glm::ivec3 nr_ptcls{100, 100, 100};
	Program render_prog, compute_prog;
	int total_ptcls;

    float time{0.f}, speed{1.f}, angle{0.f};
    glm::vec4 blhs[2] = { { 1,0,0,1}
					    , {-1,0,0,1} };

    GLuint vao_ptcl, buf_poss, buf_vels;
    GLuint vao_blh, buf_blh;
}

static void initScene() {
	render_prog.attatch("im_anims/shaders/ptcl.vs").attatch("im_anims/shaders/ptcl.fs").link();
	compute_prog.attatch("im_anims/shaders/ptcl.comp").link();

	vector<vec4> initPoss, initVels;
	total_ptcls = nr_ptcls.x*nr_ptcls.y*nr_ptcls.z;
	vec3 areaSize{2.f};
	vec3 startPos = -areaSize/2.f;
	vec3 nrSteps = vec3{nr_ptcls} - vec3{1};
	vec3 stepSize = areaSize/nrSteps;
	initPoss.reserve(total_ptcls);
	for(int z=0;z<nr_ptcls.z;z++) for(int y=0;y<nr_ptcls.y;y++) for(int x=0;x<nr_ptcls.x;x++) {
		vec3 idx{x,y,z};
		vec4 posm = vec4{startPos + stepSize*idx, 1.f};
		initPoss.push_back(posm);
	}
	initVels.resize(total_ptcls, vec4{0});


	glGenBuffers(1, &buf_poss);
	glBindBuffer(GL_ARRAY_BUFFER, buf_poss);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*initPoss.size(), initPoss.data(), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &buf_vels);
	glBindBuffer(GL_ARRAY_BUFFER,  buf_vels);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*initVels.size(), initVels.data(), GL_DYNAMIC_COPY);

	glGenVertexArrays(1, &vao_ptcl);
	glBindVertexArray(vao_ptcl);
	glBindBuffer(GL_ARRAY_BUFFER, buf_poss);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,4,GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &buf_blh);
	glBindBuffer(GL_ARRAY_BUFFER, buf_blh);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*2, blhs, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &vao_blh);
	glBindVertexArray(vao_blh);
	glBindBuffer(GL_ARRAY_BUFFER, buf_blh);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,4,GL_FLOAT, GL_FALSE, 0, 0);
}

AppParticle::AppParticle()
	: AppBase(1200, 780, APP_NAME, false), viewport(new FramebufferRbDepth())
{
	initScene();
}
AppParticle::~AppParticle()
{
}
void AppParticle::update()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.f, 0.1f, 0.12f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	compute_prog.use();
	compute_prog.setUniform("DeltaT", delta_time*0.1f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_poss);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_vels);
	glDispatchCompute(total_ptcls/nr_invocations, 1, 1);
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	viewport.getFb().bind();
	{
		render_prog.use();
		render_prog.setUniform("color", glm::vec4(1,1,1,0.2f));
		viewport.camera.setUniformTo(render_prog);

		glPointSize(1.0f);
		glBindVertexArray(vao_ptcl);
		glDrawArrays(GL_POINTS, 0, total_ptcls);
	}
	viewport.getFb().unbind();
	glDisable(GL_BLEND);

}
void AppParticle::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGuiAndUpdateCam();


	ImGui::Begin("controller##ptcl");
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}