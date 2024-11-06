/*
1 문제점:
	a. aiProcess_JoinIdenticalVertices가 작동하지 않아서 shared vertex구조를 만들수없음
	b. obj의 bunny에서는 병합 잘되는데 Model0.stl에서 안됨
	c. MeshLab에서는 Model0.stl이 shared vertex로 병합잘됨 91400 -> 15242verts, in 30480tris 
	d. Assimp에서 91440 -> 91150verts로 몇백개 정도밖에 병합되지 않음
1 접근:
	a. threashold문제이거나 두 모델의 미터 단위의 차이일거로 예상됨
	b. 우선 버니와 치아 모델의 바운더리를 출력해본다.
	c. 버니는 0.1 정도의 바운더리, 치아는 10정도의 바운더리를 가진다.
	d. 100배차이로 치아모델의 기대오차값이 커져서 중복버텍스로 인식안되는듯.
2 해결:

	b. threashold를 따로 설정할수있는 옵션을 못찾음
	c. 
*/

#include "app_curvature.h"
#include <limbrary/tools/log.h>
#include <limbrary/model_view/model.h>
#include <imgui.h>
#include <assimp/postprocess.h>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

AppCurvature::AppCurvature() : AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferMs(), "Viewport")
{
	viewport.camera.viewing_mode = CameraCtrl::ViewingMode::VM_TRACKBALL_MOVE;

	program.name = "debugging";
	program.home_dir = APP_DIR;
	program.attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();

	Model md;
	GLuint pFlags = 0;
	// pFlags |= aiProcess_Triangulate;
	// pFlags |= aiProcess_GenSmoothNormals;
	pFlags |= aiProcess_JoinIdenticalVertices;
	// pFlags |= aiProcess_CalcTangentSpace;

	// md.importFromFile("assets/models/objs/bunny.obj", false, true, 2.f, vec3(0), pFlags);
	md.importFromFile("im_curvature/models/Model0.stl", false, true, 2.f, vec3(0), pFlags);
	assert(md.own_meshes.size() == 1);
	transform = md.root.childs[0].tf;
	mesh = std::move(md.own_meshes[0]);
}
AppCurvature::~AppCurvature()
{
}
void AppCurvature::update() 
{
	viewport.getFb().bind();
	program.use();
	viewport.camera.setUniformTo(program);
	program.setUniform("mtx_Model", transform.mtx);
	mesh->bindAndDrawGL();
	viewport.getFb().unbind();


	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}
void AppCurvature::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGuiAndUpdateCam();

	ImGui::Begin("test window##curvature");
	ImGui::Text("#verts: %6d", mesh->poss.size());
	ImGui::Text("#nors:  %6d", mesh->nors.size());
	ImGui::Text("#tans:  %6d", mesh->tangents.size());
	ImGui::Text("#tris:  %6d", mesh->tris.size());
	if(ImGui::Button("asdf")) {
		log::pure("asdf");
	}
	ImGui::End();
}