/*
	Todo: 
*/

#include "app_ik.h"
#include <limbrary/tools/glim.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/limgui.h>
#include <Eigen/Dense>
#include <Eigen/SVD>

using namespace glm;
using namespace std;
using namespace lim;

namespace {


AppIK* g_app = nullptr;

int step_size = 10;
float ik_speed = 10.f;
float body_length = 1.f;
int nr_joints = 4;




struct Joint {
	Joint* parent = nullptr;
	std::vector<Joint*> connection;

	// local
	vec3 link{0};
	quat q{1,0,0,0}; // theta
	// global
	vec3 pos{0};
	quat ori{1,0,0,0};
	float max_length = -1.f;

	Joint( Joint* p, const vec3& l ) : parent(p), link(l) {
		if( !parent )
			return;
		connection.reserve(parent->connection.size()+1);
		connection = parent->connection;
		connection.push_back(this);

		max_length = 0.f;
		for(Joint* j : connection) {
			max_length += length(j->link);
		}
	}
	void rotate( const quat& rot ) {
		q = rot*q;
	}
	void update() {
		ori = parent->ori * q;
		
		// pos = parent->pos + glm::rotate(ori, link);

		quat p = ori * quat{0, link} * inverse(ori);
		pos = parent->pos + vec3{p.x,p.y,p.z};
	}
	void draw() const {
		g_app->drawCylinder(parent->pos, pos, 0.025f, {1,1,0});
		g_app->drawSphere( pos, 0.05f, {1,1,0} );
	}
	void solveIK(const vec3& targetPos, float dt) {
		dt = ik_speed * dt/step_size;

		// 힌지가 볼엔소캣이기 때문에 xyz축으로 각각 돌린다.
		const vector<vec3> axes = {{1,0,0},{0,1,0},{0,0,1}};
		// const vector<vec3> axes = {{1,0,0},{0,1,0}};
		const int nr_axes = (int)axes.size();
		const int connectionSize = int(connection.size());

		// constraints 	: x(x,y,z)
		// freedoms 	: nrJoints*3
		Eigen::MatrixXf J{3, connectionSize*nr_axes};
		Eigen::MatrixXf b{3, 1};
		Eigen::VectorXf dThetas{connectionSize*3};

		for( int step=0; step<step_size; step++ ) {
			vec3 d = targetPos - pos;
			b << d.x, d.y, d.z;
			int curCol = 0;
			for( const Joint* curJnt : connection ) {
				vec3 p = curJnt->pos - curJnt->parent->pos;
				for( const vec3& axis : axes ) {
					vec3 v = cross( axis, p );
					J.col( curCol++ ) << v.x, v.y, v.z;
				}
			}
			// 수도 인버스 qr, 하우스 홀더는 알고리즘이다.
			// <QR>
			// least square 솔루션을 계산할수있지만 싱귤러해질때 문제가생긴다.
			// dThetas = J.colPivHouseholderQr().solve(b);
			// <SVD>
			// u 와 v는 필요없어서 작게 계산하도록
			// Todo: 얼마나 싱귤러한지 threshold를 설정하는것 찾아보기
			dThetas = J.bdcSvd(Eigen::ComputeThinU|Eigen::ComputeThinV).solve(b);
			curCol = 0;
			for( Joint* curJnt : connection ) {
				for( const vec3& axis : axes ) {
					float amout = dt*dThetas[curCol++];
					curJnt->rotate( glim::exp(quat(0, amout*axis)) );
					curJnt->update();
				}
			}
		}
	}
};




struct Body {
	Joint root; // for pos
	vector<Joint> joints;

	Body() : root(nullptr, vec3{0}) {
	}
	void setJoints() {
		const float linkLength = body_length/nr_joints;
		joints.clear();
		joints.reserve(nr_joints);
		joints.emplace_back( &root, vec3{0,linkLength,0} );
		for( int i=0; i<nr_joints-1; i++ ) {
			joints.emplace_back( &joints.back(), vec3{0,linkLength,0} );
		}
	}
	void setLength() {
		const float linkLength = body_length/nr_joints;
		for( auto& j: joints ) {
			j.link = vec3{0,linkLength,0};
		}
	}
	void draw() const {
		g_app->drawSphere( vec3{0}, 0.05f, {1,1,0} );
		for( const auto& j: joints ) {
			j.draw();
		}
	}
	void update() {
		for( auto& j: joints ) {
			j.update();
		}
	}
};



Body body;
Joint* target_jnt = nullptr;
vec3 target_pos{0};



}  	// end anonymous namespace







static void resetScene() {
	body.setJoints();
	// body.setLength();
}

AppIK::AppIK() : AppBaseCanvas3d(1200, 780, APP_NAME, true) {
	g_app = this;
	resetScene();
}
AppIK::~AppIK() {
}
void AppIK::canvasUpdate() {
	if( target_jnt ) {
		float targetPosLength = length( target_pos - body.root.link );
		if( targetPosLength > target_jnt->max_length ) {
			target_pos = body.root.link + (target_pos * target_jnt->max_length/targetPosLength);
		}
		target_jnt->solveIK( target_pos, delta_time );
	}
	body.update();
}
void AppIK::canvasDraw() const {
	if( target_jnt ) {
		drawSphere(target_pos, 0.03f, {1,0,0});
	}

	body.draw();

	// axis object 10cm
	constexpr vec3 ori{1,0,0};
	drawCylinder(ori, ori+vec3{0.1f,0,0}, 0.025f, {1,0,0});
	drawCylinder(ori, ori+vec3{0,0.1f,0}, 0.025f, {0,1,0});
	drawCylinder(ori, ori+vec3{0,0,0.1f}, 0.025f, {0,0,1});

	drawQuad({0,0,0}, {0,1,0}, vec2{100,100}, {0.f, 0.3f, 0.f});
}

void AppIK::canvasImGui() {

	ImGui::Begin("test window##ik");
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::SliderInt("step size", &step_size, 1, 200);
	ImGui::SliderFloat("ik speed", &ik_speed, 1.f, 15.f);
	if(ImGui::SliderInt("nr joints", &nr_joints, 1, 10)) {
		body.setJoints();
	}
	if(ImGui::SliderFloat("body length", &body_length, 0.2f, 2.5f)) {
		body.setLength();
	}
	ImGui::End();


	if( ImGui::IsMouseClicked(ImGuiMouseButton_Right, false) ) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		const vec3 cameraPos = vp.camera.pos;
		float minDepth = FLT_MAX;
		for(int i=0; i<body.joints.size(); i++) {
			const vec3& objP = body.joints[i].pos;
			vec3 toObj = objP - cameraPos;
			float distFromLine = glm::length( glm::cross(mouseRay, toObj) );
			float distProjLine = glm::dot(mouseRay, toObj);

			if( distFromLine < 0.02f ) {
				if( distProjLine>0 && minDepth>distProjLine ) {
					minDepth = distProjLine;
					target_jnt = &body.joints[i];
				}
			}
		}
	}
	else if( target_jnt && ImGui::IsMouseDown(ImGuiMouseButton_Right) ) {
		const vec3 toObj = target_pos-vp.camera.pos;
		const vec3 mouseRay = vp.getMousePosRayDir();
		const float depth = dot(vp.camera.front, toObj)/dot(vp.camera.front, mouseRay);
		target_pos = depth*mouseRay+vp.camera.pos;
	}
	else if( ImGui::IsMouseReleased(ImGuiMouseButton_Right) ) {
		target_jnt = nullptr;
	}
}