/*
	Todo: 
*/

#include "app_kinematics.h"
#include <limbrary/log.h>
#include <limbrary/limgui.h>
#include <Eigen/core>
#include <Eigen/Dense>

using namespace glm;
using namespace std;
using namespace lim;

namespace {
	AppKinematics* g_app = nullptr;

	int step_size = 10;
	float ik_speed = 3.f;
	float body_length = 1.f;
	int nr_joints = 4;

	// glm::exp(quat) fixed version
	inline quat myexp(const quat& q) {
		const vec3 u{q.x, q.y, q.z};
		const float Angle = length(u);
		if( Angle < epsilon<float>() ) {
			return {cos(Angle), sin(Angle)*u};
		}
		const vec3 v{u/Angle};
		return {cos(Angle), sin(Angle)*v};
	}
}

struct Joint {
	Joint* parent = nullptr;
	std::vector<Joint*> connection;

	// local
	vec3 link{0};
	quat q{1,0,0,0}; // theta
	// global
	vec3 pos{0};
	quat ori{1,0,0,0};

	Joint( Joint* p, const vec3& l ) : parent(p), link(l) {
		if( !parent )
			return;
		connection.reserve(parent->connection.size()+1);
		connection = parent->connection;
		connection.push_back(this);
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
		g_app->drawCylinder(parent->pos, pos, {1,1,0});
		g_app->drawSphere( pos, {1,1,0} );
	}
	void solveIK(const vec3& target_pos, float dt) {
		dt = ik_speed * dt/step_size;

		// 힌지가 볼엔소캣이기 때문에 xyz축으로 각각 돌린다.
		const vector<vec3> axes = {{1,0,0},{0,1,0},{0,0,1}};
		// const vector<vec3> axes = {{1,0,0},{0,1,0}};
		const int nr_axes = axes.size();
		const int connectionSize = int(connection.size());

		// constraints 	: x(x,y,z)
		// freedoms 	: nrJoints*3
		Eigen::MatrixXf J{3, connectionSize*nr_axes};
		Eigen::MatrixXf b{3, 1};
		Eigen::VectorXf dThetas{connectionSize*3};

		for( int step=0; step<step_size; step++ ) {
			vec3 d = target_pos - pos;
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
			// 얼마나 싱귤러한지 threshold를 설정하는것 찾아보기
			dThetas = J.bdcSvd(Eigen::ComputeThinU|Eigen::ComputeThinV).solve(b);
			curCol = 0;
			for( Joint* curJnt : connection ) {
				for( const vec3& axis : axes ) {
					float amout = dt*dThetas[curCol++];
					curJnt->rotate( myexp(quat(0, amout*axis)) );
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
		g_app->drawSphere( vec3{0}, {1,1,0} );
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



namespace {
	Body body;
	Joint* target_jnt = nullptr;
	vec3 target_pos{0};
}

static void resetScene() {
	body.setJoints();
	// body.setLength();
}

AppKinematics::AppKinematics() : AppBaseCanvas3d(1200, 780, APP_NAME, true) {
	g_app = this;
	resetScene();
}
AppKinematics::~AppKinematics() {
}
void AppKinematics::canvasUpdate() {
	if( target_jnt ) {
		target_jnt->solveIK( target_pos, delta_time );
	}
	body.update();
}
void AppKinematics::canvasDraw() const {
	if( target_jnt ) {
		drawSphere(target_pos, {1,0,0}, 0.03f);
	}

	body.draw();

	// axis object 10cm
	constexpr vec3 ori{1,0,0};
	drawCylinder(ori, ori+vec3{0.1f,0,0}, {1,0,0});
	drawCylinder(ori, ori+vec3{0,0.1f,0}, {0,1,0});
	drawCylinder(ori, ori+vec3{0,0,0.1f}, {0,0,1});

	drawQuad({0,0,0}, {0,1,0}, {0,0.3f,0});
}

void AppKinematics::canvasImGui() {
	log::drawViewer("logger##ik");

	ImGui::Begin("test window##ik");
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::SliderInt("step size", &step_size, 1, 200);
	ImGui::SliderFloat("ik speed", &ik_speed, 1.f, 10.f);
	if(ImGui::SliderInt("nr joints", &nr_joints, 1.f, 10)) {
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
					target_pos = objP;
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