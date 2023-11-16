/*
 	Note:

	Todo:
	glsl uniform 배열들 texture에 저장해서 넘기기
*/

#include "app_sdf_modeler.h"
#include "sdf_lib.h"
#include <limbrary/asset_lib.h>
#include <imgui.h>


lim::AppSdfModeler::AppSdfModeler(): AppBase(1373, 783, APP_NAME, false)
	, viewport("viewport", new FramebufferNoDepth()) // 멀티셈플링 동작 안함.
{
	GLint tempInt;
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &tempInt);
	log::pure("GL_MAX_FRAGMENT_UNIFORM_VECTORS : %d\n", tempInt);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &tempInt);
	log::pure("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS : %d\n", tempInt);

	viewport.use_guizmo = true;
	viewport.camera.pivot = glm::vec3(0,1,0);
	viewport.camera.position = glm::vec3(0,1,5);
	viewport.camera.updateViewMat();
	prog.name = "sdf and ray marching";
	prog.home_dir = APP_DIR;
	prog.attatch("canvas.vs").attatch("shader.fs").link();



	sdf::init();
}
lim::AppSdfModeler::~AppSdfModeler()
{
	sdf::deinit();
}
void lim::AppSdfModeler::update()
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Camera& cam = viewport.camera;
	viewport.getFb().bind();
	prog.use();
	prog.setUniform("lightPos", light.position);
	prog.setUniform("lightInt", light.intensity);
	prog.setUniform("cameraAspect", cam.aspect);
	prog.setUniform("cameraFovy", cam.fovy);
	prog.setUniform("cameraOrthWidth", 0.f);
	prog.setUniform("cameraPos", cam.position);
	prog.setUniform("cameraPivot", cam.pivot);
	sdf::bindSdfData(prog);

	AssetLib::get().screen_quad.drawGL();
	viewport.getFb().unbind();
}
void lim::AppSdfModeler::renderImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGui([&cam = viewport.camera](const Viewport& vp){
		sdf::drawGuizmo(vp, cam);
	});

	sdf::drawImGui();
}
void lim::AppSdfModeler::keyCallback(int key, int scancode, int action, int mods) {
	if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
		prog.reload(GL_FRAGMENT_SHADER);
	}
}