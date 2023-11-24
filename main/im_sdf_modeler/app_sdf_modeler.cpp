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

	ImGui::GetIO().Fonts->AddFontFromFileTTF("im_sdf_modeler/fonts/SpoqaHanSansNeo-Medium.ttf", 16.0f);

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f;
    static ImWchar lIconRanges[] = { 0xE800, 0xE840, 0 };
    ImGui::GetIO().Fonts->AddFontFromFileTTF("im_sdf_modeler/fonts/icons.ttf", 20.f, &config, lIconRanges);


	viewport.camera.pivot = glm::vec3(0,1,0);
	viewport.camera.position = glm::vec3(0,1,5);
	viewport.camera.updateViewMat();
	prog.name = "sdf and ray marching";
	prog.home_dir = APP_DIR;
	prog.attatch("canvas.vs").attatch("shader.fs").link();

	sdf::init(&viewport.camera, &light);
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
	sdf::bindSdfData(prog);

	AssetLib::get().screen_quad.drawGL();
	viewport.getFb().unbind();
}
void lim::AppSdfModeler::renderImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGui([](const Viewport& vp){
		sdf::drawGuizmo(vp);
	});

	sdf::drawImGui();
}
void lim::AppSdfModeler::keyCallback(int key, int scancode, int action, int mods) {
	if( action==GLFW_PRESS && GLFW_MOD_CONTROL== mods && key=='R' ) {
		prog.reload(GL_FRAGMENT_SHADER);
	}
}
void lim::AppSdfModeler::dndCallback(int count, const char **paths) {
	sdf::dndCallback(count, paths);
}