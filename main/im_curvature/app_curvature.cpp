/*
day 1
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

day 2
2 Curvature:
	https://en.wikipedia.org/wiki/Mean_curvature
	https://www.youtube.com/watch?v=9C_6TCRS13Q&t=53s
	https://www.youtube.com/watch?v=3fZia2qSk-o&t=16s
	주principal곡률 k1최대, k2최소

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

	k1, k2구할때 quadric method
		computeReferenceFrames
			vert face iterator로

	vcg라이브러리 참고해서 quadric method로 주곡률방향과 값구하고 평균곡률구해서 렌더링했다.
	중간중간 튀는값이 있는데 shared vertex가 잘안잡힌건지 디버깅이 잘안되어서 일단 패스

day 3
3 Picking:
	현제 Mesh의 모델공간에서 월드공간의[-1, 1]로 정규화해서 그리고있다.
	피킹할때 우선 마우스위치, 카메라 위치, 방향을 가지고 클릭한 레이을 알아내고
	모든버텍스를 순회하는데, 월드공간에서 거리를 측정하면 공간변환연산이 o(n)번발생하므로
	카메라를 모델공간으로 옮겨 측정한다. 우선 레이직선과 버텍스사이거리로 걸러내고 카메라와의 깊이중 가장 가까운것을 찾는다.
	이때 threshold를 mesh의 모델공간에 비례해서 설정해줘야한다. Todo
	
	Triangle Picking도 일단 구현해두었다.

5. mean curvature 기준 클러스터링 및 Mesh추출
	bfs로 선택한 버텍스부터 주변 버텍스를 탐색하며 플레그를 설정하고
		false depth로 연속된 threshold 탈락을 몇번까지 허용할지 옵션을 줬다.
	face index정보로 플레그를 수정하며 face를 선택하고
	새로운 버텍스 배열을 생성하고
	선택된 face들의 인덱스를 수정한다.
6. 표면에서 화살표 기즈모
	imgui gizmo라이브러리 두개의 코드를 비교해보았는데
	1. imguizmo: 2d화면공간으로 좌표로 옮겨서 imgui draw list에 커맨드를 추가한다.
		문제점: 3d가아니라 화면을 회전해보지 않고 정확한 방향을 인지하기 어렵다.
		장점: 드래그입력
	2. imguizmo.quat: ndc에 큐브 실린더 스피어 콘 등 버텍스정보를 만들어두고
		draw호출마다 쿼터니언으로 돌리고 스크린스페이스로 옮긴다음 imgui draw list의 PrimVtx에 추가한다. 이때 쉐이딩도 cpu에서 버텍스별로한다.
		쿼터니언으로 연산줄인 부분이 신기한데 메번 버텍스를 cpu에서 계산해서 추가하는 방법이 복잡하고
		뒤에 가렸을때 등을 처리하지 못함. 그리고 기본도형 버텍스위치 계산하는 코드등 중복코드가 생긴다.
	limbrary의 codemesh를 사용해서 model matrix를 계산해서 인스턴스 드로잉하는게 더 빠를것같아 limgizmo를 만들어보려고함
		목표: 
			1.가려진부분 투명처리
			2.스케일은 화면공간에서 고정
			3.drawArrowGizmo(wpos, wdir, slen) // screen space length
		스케일고정은 모델행렬에서 스케일을 동적으로 fov랑 거리 비례해서
*/

#include "app_curvature.h"
#include <limbrary/tools/log.h>
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/transform.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/gizmo.h>
#include <imgui.h>
#include <assimp/postprocess.h>
#include <glm/gtx/transform.hpp>
#include "curvature.h"

#include <fstream>
#include <filesystem>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;


namespace {
	Transform ms_tf;

	int picked_t_idx;
	int picked_v_idx;
	bool is_draw_face_nor = true;

	struct ArrowGizmo {
		int idx;
		vec3 wpos, wdir;
	};
	std::vector<ArrowGizmo> arrow_gizmos(10);
}

namespace abut{
	GLuint abut_vbo, abut_vao;
	std::vector<vec3> abut_verts;
	Program* abut_prog;

	void init()
	{
		abut_prog = new Program("line");
		abut_prog->home_dir = AppCurvature::APP_DIR;
		abut_prog->attatch("line.vs").attatch("line.fs").link();

		abut_verts.reserve(278);

		std::ifstream input("im_curvature/models/abutvertex.xyz");
		assert(input.is_open());
		while(!input.eof()) {
			vec3 v;
			input >> v.x >> v.y >> v.z;
			abut_verts.emplace_back(v);
		}
		glGenVertexArrays(1, &abut_vao);
		glBindVertexArray(abut_vao);

		glGenBuffers(1, &abut_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, abut_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*abut_verts.size(), abut_verts.data(), GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 
	}

	void deinit()
	{
		glDeleteVertexArrays(1, &abut_vao);
		glDeleteBuffers(1, &abut_vbo);
		delete abut_prog;
	}

	void draw(const lim::Camera& cam, const mat4& model)
	{
		abut_prog->use();
		cam.setUniformTo(*abut_prog);
		abut_prog->setUniform("mtx_Model", model);
		abut_prog->setUniform("color", vec4(1,0,1,1));
		glBindVertexArray(abut_vao);
		glDrawArrays(GL_LINE_STRIP, 0, abut_verts.size());
	}
}





AppCurvature::AppCurvature() : AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferMs(), "Viewport")
{
	picked_t_idx = -1;
	picked_v_idx = -1;
	arrow_gizmos.clear();

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
	ms_tf = md.root.childs[0].tf;
	mesh = std::move(md.own_meshes[0]);
	mesh->cols.resize(mesh->poss.size(), vec3(-1,0,0));
	mesh->initGL();

	curv::uploadMesh(*mesh);

	gizmo::init();

	abut::init();
}
AppCurvature::~AppCurvature()
{
	gizmo::deinit();

	abut::deinit();
}

void AppCurvature::update() 
{
	viewport.getFb().bind();
	program.use();
	viewport.camera.setUniformTo(program);

	// model
	program.setUniform("color", vec4(1));
	program.setUniform("is_DrawValue", curv::isComputedCurv());
	program.setUniform("is_DrawFaceNor", is_draw_face_nor);
	program.setUniform("mtx_Model", ms_tf.mtx);
	if( rst_ms==nullptr ) {
		mesh->bindAndDrawGL();
	}
	else {
		rst_ms->bindAndDrawGL();
	}
	abut::draw(viewport.camera, ms_tf.mtx);

	// picking gizmo
	if(picked_v_idx >= 0) {
		gizmo::drawPoint(ms_tf.mtx*vec4(mesh->poss[picked_v_idx],1), vec4(1,0,1,1), 0.05f, viewport.camera, false);
	} else if(picked_t_idx >= 0) {
	}

	for(auto& gm : arrow_gizmos) {
		float len = 5*ms_tf.scale.x;
		gizmo::drawArrow(gm.wpos, gm.wdir, vec4(1,0,0,1), len, 0.05f, viewport.camera, false);
	}


	viewport.getFb().unbind();


	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int AppCurvature::pickVertIdx(glm::vec3 rayOri, const glm::vec3 &rayDir) 
{
	float minDepth = FLT_MAX;
	int rst = -1;

	// rot없어서 dir수정안함
	// uniform scale이라 연산 더 줄일수있긴함
	rayOri = glm::inverse(ms_tf.mtx) * vec4(rayOri, 1.f);
	// log::pure(rayOri);
	// log::pure(rayDir);

	for (int i = 0; i < mesh->nr_verts; i++) {
		const vec3 toObj = mesh->poss[i] - rayOri;
		float distFromLine = glm::length(glm::cross(rayDir, toObj));
		if( distFromLine < 0.1f ) {
			float distProjLine = glm::dot(rayDir, toObj);
			if( distProjLine > 0 && distProjLine < minDepth ) {
				minDepth = distProjLine;
				rst = i;
			}
		}
	}

	return rst;
}

std::pair<int, vec3> AppCurvature::pickTri(glm::vec3 rayOri, const glm::vec3& rayDir) {
	const float searchDepth = 500.f;
	float minDepth = FLT_MAX;
	int rst = -1;

	rayOri = glm::inverse(ms_tf.mtx) * vec4(rayOri, 1.f);


	for(int i=0; i<mesh->nr_tris; i++) {
		const ivec3& tri = mesh->tris[i];
		const vec3& v1 = mesh->poss[tri.x];
		const vec3& v2 = mesh->poss[tri.y];
		const vec3& v3 = mesh->poss[tri.z];
		
		float depth = glim::intersectTriAndRayBothFaces(rayOri, rayDir, v1, v2, v3);
		if( depth<0 || depth>searchDepth || depth>minDepth )
			continue;
		minDepth = depth;
		rst = i;
	}

	return {rst, minDepth*rayDir + rayOri};
}

void AppCurvature::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGuiAndUpdateCam([](ViewportWithCam& vp) {
		// ImVec2 window_pos = ImGui::GetWindowPos();
		// ImVec2 window_size = ImGui::GetWindowSize();
		// ImVec2 window_center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
		// ImGui::GetWindowDrawList()->AddCircle(ImGui::GetMousePos(), window_size.y * 0.6f, IM_COL32(0, 255, 0, 200), 0, 10);
		// ImGui::GetWindowDrawList()->AddConvexPolyFilled
	});

	if( ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) ) {
		const vec3 mouseRay = viewport.getMousePosRayDir();
		int idx = pickVertIdx(viewport.camera.pos, mouseRay);
		if( idx >= 0 ) {
			picked_v_idx = idx;
			picked_t_idx = -1;
			// log::pure("%d\n", picked_v_idx);
		}
	}
	else if( ImGui::IsMouseClicked(ImGuiMouseButton_Right, false) ) {
		const vec3 mouseRay = viewport.getMousePosRayDir();
		auto [idx,p] = pickTri(viewport.camera.pos, mouseRay);
		if( idx >= 0 ) {
			picked_t_idx = idx;
			picked_v_idx = -1;
			// log::pure("%d\n", picked_t_idx);
		}
	}
	else if( ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false) ) {
		const vec3 mouseRay = viewport.getMousePosRayDir();
		int idx = pickVertIdx(viewport.camera.pos, mouseRay);
		if( idx >= 0 ) {
			int gIdx = -1; // dup gizmo idx
			for(int i=0; i<arrow_gizmos.size(); i++) {
				if( arrow_gizmos[i].idx == idx ) {
					gIdx = i;
					break;
				}
			}
			vec3 dir = -viewport.camera.front;
			if( gIdx >= 0 ) {
				arrow_gizmos[gIdx].wdir = dir;
			}
			else {
				vec3 pos = ms_tf.mtx * vec4(mesh->poss[idx], 1.f);
				arrow_gizmos.push_back({idx, pos, dir});
			}
		}
	}
	ImGui::Begin("test window##curvature");
	
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Mesh info") ) {
		ImGui::Text("#verts: %6d", mesh->poss.size());
		ImGui::Text("#nors:  %6d", mesh->nors.size());
		ImGui::Text("#tans:  %6d", mesh->tangents.size());
		ImGui::Text("#tris:  %6d", mesh->tris.size());
	}
	ImGui::Dummy({0.f, 20.f});


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("Functions") ) {
		ImGui::Checkbox("draw face nor", &is_draw_face_nor);
		ImGui::Dummy({0.f, 10.f});

		if(ImGui::Button("1. update curvature")) {
			curv::computeCurvature();
			curv::downloadCurvature(*mesh);
			mesh->initGL();
		}
		ImGui::Dummy({0.f, 10.f});

		if( picked_v_idx < 0 || !curv::isComputedCurv() ) {
			ImGui::Text("2. pick vertex with left click ");
		}
		else {
			ImGui::Text("2. pick vertex: %d", picked_v_idx);
			vec3 info = mesh->cols[picked_v_idx];
			ImGui::Text("vert idx: %d", picked_v_idx);
			ImGui::Text("mean curvature: %f", info.r);
			ImGui::Text("k1, k2: %f, %f", info.g, info.b);
			ImGui::Dummy({0.f, 10.f});
			
			static float cluster_threshold = 0.8f;
			static int max_false_depth = 2;
			ImGui::SliderInt("max false depth", &max_false_depth, 0, 5);
			ImGui::SliderFloat("threshold", &cluster_threshold, 0.2f, 1.f);
			if(ImGui::Button("3. separate mesh")) {
				rst_ms = curv::getClusteredMesh(picked_v_idx, cluster_threshold, max_false_depth);
			}
			if(ImGui::Button("4. reset")) {
				rst_ms = nullptr;
			}
		}
	}
	
	ImGui::End();
}