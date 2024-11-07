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
	c. 디버깅
		1. postprocessing옵션에 따라 process set가 정해져있음 JoinIdenticalVertces는
			1. ComputeSpatialSortProcess
			2. JoinVerticesProcess
			3. DestroySpatialSortProcess
			으로 실행됨.
		2. 두번째 과정에서 threashold가 사용될거로 예상됨.
		3. 확인해보니 단순 상수 threashold를 사용하여 버텍스 속성들로 비교하며 값을 가지고 있었음
			bool areVerticesEqual(...) in JoinVerticesProcess.cpp line102
		4. 내 생각에 바운더리에 비례해서 threashold값을 수정하거나 사용자가 직접 입력해야 좋을것같음
			Todo: 이슈, PR등록
		5. 우선 핵심 문제가 아니므로 상수값을 100배 높여 테스트해보자. 100배로 부족해다
		6. 10프로대로 줄여질때까지 수동으로 맞추었더니 기존 1-e5f 에서 0.09f로 올라갔다.

2 Curvature ref:
	https://en.wikipedia.org/wiki/Mean_curvature
	주principal곡률 k1최대, k2최소
	평균은 곡면이 

	a. 필터링 옵션
		Method : Quadric
		Quality/Color mapping : mean curvature
		curvature scale : 1.76(world unit), 10(perc on)
	b. 필터링 부분은 vtk라이브러리 내용
		UpdateCurvatureFitting::computeCurvature // quadric
			CompactVertexVector
			RequireCompactness
			RequireVFAdjacency
			Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d>
			요약
				인접정보 생성하고
				Eigen으로 k1 k2구함
		UpdateQuality::VertexMeanFromCurvatureDir // mean curvature
			k1 k2평균으로 곡률업데이트
		2변수 2차함수 quadric
			a*x*x + b*x*y + c*y*y + d*x + e*y
			상수값 없음
		CMeshO::CoordType는 vec3
		computeCurvature에서 버텍스순회 안에서
			computeReferenceFrames // 첫번째 인접삼각형 다음버텍스로 인접버텍스를 찾고

3. 문제점:
	MeshLab의 
	tri::UpdateCurvatureFitting<CMeshO>::computeCurvature(m.cm) // quadric meshod
	tri::UpdateQuality<CMeshO>::VertexMeanFromCurvatureDir(m.cm)
	위 두개의 함수의 구현부가 소스코드에 없음
	VertexMeanFromCurvatureDir, 
*/

#include "app_curvature.h"
#include <limbrary/tools/log.h>
#include <limbrary/model_view/model.h>
#include <imgui.h>
#include <assimp/postprocess.h>
#include "curvature.h"

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

AppCurvature::AppCurvature() : AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferMs(), "Viewport")
{
	viewport.camera.viewing_mode = CameraCtrl::ViewingMode::VM_TRACKBALL_MOVE;

	program.name = "debugging";
	program.home_dir = APP_DIR;
	program.attatch("debug.vs").attatch("debug.fs").link();

	Model md;
	GLuint pFlags = 0;
	// pFlags |= aiProcess_Triangulate;
	// pFlags |= aiProcess_GenNormals;
	// pFlags |= aiProcess_GenSmoothNormals;
	pFlags |= aiProcess_JoinIdenticalVertices;
	GLuint pFlags2 = 0;
	// stl doesn't have uv so we can't calc tangent
	// pFlags2 |= aiProcess_CalcTangentSpace;

	// md.importFromFile("assets/models/objs/bunny.obj", false, true, 2.f, vec3(0), pFlags, pFlags2);
	md.importFromFile("im_curvature/models/Model0.stl", false, true, 2.f, vec3(0), pFlags, pFlags2, false);
	assert(md.own_meshes.size() == 1);
	transform = md.root.childs[0].tf;
	mesh = std::move(md.own_meshes[0]);
	mesh->cols.resize(mesh->poss.size(), vec3(-1,0,0));
	mesh->initGL();

	curv::uploadMesh(*mesh);
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
	if(ImGui::Button("update curvature")) {
		curv::computeCurvature();
		curv::downloadCurvature(*mesh);
		mesh->initGL();
	}
	ImGui::End();
}