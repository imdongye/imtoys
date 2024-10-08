/*
	2024-08-05 / im dongye
	Todo: 
	1. joint hierarchy view
	2. interplate keys;
*/

#include "app_bvh_parsor.h"
#include <limbrary/tools/glim.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/limgui.h>
#include <fstream>
#include <filesystem>

using namespace glm;
using namespace std;
using namespace lim;

namespace {

struct Motion {
	int nr_frames, nr_joints;
	float frame_dt;
	float running_time;
	// nrFrames * nrJoints * nrCannels
	vector<vector<vector<float>>> frames;
};



struct Joint {
	enum CHANNEL : int {
		X_POS, Y_POS, Z_POS,
		X_ROT, Y_ROT, Z_ROT,
	};
	int parent_id = -1;
	int id = -1;
	string name = "";
	vec3 link, position;
	quat q, orientation;
	int nr_channels;
	vector<CHANNEL> channers;

	void poseByKeys(const vector<float>& keys) {
		q = quat(1, 0, 0, 0);
		for( int i=0; i<nr_channels; i++ ) {
			CHANNEL c = channers[i];
			float element = keys[i];
			switch (c) { // ZXY
			case Z_ROT: q *= glim::rotateV({0,0,1}, element); break;
			case X_ROT: q *= glim::rotateV({1,0,0}, element); break;
			case Y_ROT: q *= glim::rotateV({0,1,0}, element); break;
			case X_POS: link.x = element; break;
			case Y_POS: link.y = element; break;
			case Z_POS: link.z = element; break;
			}
		}
	}

	void update(const vec3& parentP, const quat& parentQ, float scale) {
		quat l = quat(0, scale*link);
		quat p = parentQ * l * inverse(parentQ);
		position = parentP + vec3(p.x, p.y, p.z);
		orientation = parentQ * q;
	}
};




struct Body {
	int nr_joints = 0;
	vector<Joint> joints;
	vec3 position = {0,0,0};
	float scale = 1.f;

	void draw(const AppBvhParsor& app) {
		for(auto& j : joints) {
			vec3 p = j.position;
			app.drawSphere(p, 0.05f, {1.f, 0.5f, 0.f});
			if( j.parent_id >= 0 ) {
				vec3 pp = joints[j.parent_id].position;
				app.drawCylinder(p, pp, 0.025f, {1.f, 0.7f, 0.f});
			}
		}
	}


	void updateJointsGlobal() {
		joints[0].update(position, quat(1, 0, 0, 0), scale);
		for(int i = 1; i < joints.size(); i++) {
			Joint& j = joints[i];
			const Joint& p = joints[j.parent_id];
			j.update(p.position, p.orientation, scale);
		}
	}


	void setPose(const vector<vector<float>>& frame) {
		for(int i = 0; i < nr_joints; i++) {
			Joint& joint = joints[i];
			joint.poseByKeys(frame[i]);
		}
		updateJointsGlobal();
	}

	int getJointLevel(const int parentId) {
		if( parentId<0 )
			return 0;
		return getJointLevel(joints[parentId].parent_id) + 1;
	}
};







void parceBvh(Body** ppBody, Motion** ppMotion, const char* path) {
	string token;

	*ppBody = new Body();
	*ppMotion = new Motion();

	Body& body = **ppBody;
	Motion& motion = **ppMotion;

	ifstream input(path);
	if (!input.is_open()) {
		fprintf(stderr, "Noinput file %s", path);
		assert(0);
	}

	// parsing bone hierarchy ==============
	input >> token; // HIERARCHY
	input >> token; // ROOT
	function<void(int, bool)> parceJoint;
	parceJoint = [&parceJoint, &body, &input](int parentId, bool isEnd)
	{
		string token;
		body.joints.push_back({});
		int curId = body.nr_joints++;
		Joint& joint = body.joints.back();
		joint.parent_id = parentId;

		if( isEnd == false ) {
			input >> joint.name; // name
			input >> token; // {
		}
		else {
			input >> token; // Site
			input >> token; // name or empty
			if (token == "{") {
				joint.name = body.joints[parentId].name + "_end";
			}
			else {
				joint.name = token;
				input >> token; // {
			}
		}

		int jntLev = body.getJointLevel(parentId);
		for( int i = 0; i < jntLev; i++ )
			log::pure("\t");
		log::pure("%s\n", joint.name.c_str());


		input >> token; // offset  
		if( token == "OFFSET" ) {
			input >> joint.link.x;
			input >> joint.link.y;
			input >> joint.link.z;
		}

		if( isEnd ) {
			input >> token; // }
			return;
		}

		input >> token; // channels  
		if( token == "CHANNELS" ) {
			input >> joint.nr_channels;
			for(int i = 0; i < joint.nr_channels; i++) {
				input >> token; // rotation // Z X Y
				if     (token == "Zrotation") joint.channers.push_back(Joint::CHANNEL::Z_ROT);
				else if(token == "Xrotation") joint.channers.push_back(Joint::CHANNEL::X_ROT);
				else if(token == "Yrotation") joint.channers.push_back(Joint::CHANNEL::Y_ROT);
				else if(token == "Xposition") joint.channers.push_back(Joint::CHANNEL::X_POS);
				else if(token == "Yposition") joint.channers.push_back(Joint::CHANNEL::Y_POS);
				else if(token == "Zposition") joint.channers.push_back(Joint::CHANNEL::Z_POS);
			}
		}


		// not ensure joint is available
		// because vector data maybe moved
		do {
			input >> token;
			if( token == "JOINT" ) {
				parceJoint(curId, false);
			}
			else if( token == "End" ) {
				parceJoint(curId, true);
			}
		} while (token != "}");
	};
	parceJoint(-1, false);
	// end parsing bone hierarchy ==============




	// parsing motion ========================
	input >> token; // MOTION
	input >> token; // Frames:
	input >> motion.nr_frames;
	input >> token; // Frame 
	input >> token; // Time:
	input >> motion.frame_dt;
	motion.running_time = motion.nr_frames * motion.frame_dt;

	log::pure("Frames: %d\n", motion.nr_frames);
	log::pure("Frame Time: %f\n", motion.frame_dt);
	log::pure("joints: %d\n", body.nr_joints);

	motion.frames.resize(motion.nr_frames);

	for(int i = 0; i < motion.nr_frames; i++)
	{
		vector<vector<float>>& frame = motion.frames[i];
		frame.resize(body.nr_joints);

		for(int j = 0; j < body.nr_joints; j++)
		{
			Joint& joint = body.joints[j];
			vector<float>& keys = frame[j];
			keys.resize(joint.nr_channels);
			for(int k = 0; k < joint.nr_channels; k++)
			{
				input >> keys[k];
			}
		}
	}
	// end parsing motion ========================



	int nrConstraints = 0;
	input >> token; // Todo: CONSTRAINTS
	if( token == "CONSTRAINTS" )
		log::pure("!! parse success !!\n");
	input >> nrConstraints; 
	for(int i = 0; i < nrConstraints; i++) {
		input >> token; // name
		input >> token; input >> token; // ?
		input >> token; // ?
		input >> token; input >> token; input >> token; // ?
	}


	int nrTags = 0;
	input >> token; // TAGS
	input >> nrTags;
	for(int i = 0; i < nrTags; i++) {
		input >> token; // name
		// ...
	}

	input.close();



	// auto scale and position =====================
	assert( motion.nr_frames>0 );
	body.setPose(motion.frames[0]);
	vec3 minPos, maxPos, boundary;
	minPos = glim::maximum_vec3;
	maxPos = glim::minimum_vec3;
	for( const Joint& j : body.joints ) {
		minPos = glm::min(j.position, minPos);
		maxPos = glm::max(j.position, maxPos);
	}
	boundary = maxPos - minPos;
	float height = 1.f;
	float curHeight = boundary.y;
	body.scale = height/curHeight;


	minPos = glim::maximum_vec3;
	maxPos = glim::minimum_vec3;
	for( const auto& frame: motion.frames ) {
		const auto& rootKeys = frame[0];
		body.joints[0].poseByKeys(rootKeys);
		minPos = glm::min(body.joints[0].link, minPos);
		maxPos = glm::max(body.joints[0].link, maxPos);
	}
	boundary = maxPos - minPos;
	body.position = -(minPos + boundary * 0.5f)*body.scale;
	body.position.y = 0;

	body.setPose(motion.frames[0]);
	// auto scale and position =====================
}



Body* g_body;
Motion* g_motion;
float elapsed_time = 0.f;
bool is_paused = false;



} // end anonymouse namespace

AppBvhParsor::AppBvhParsor()
	: AppBaseCanvas3d(1200, 780, APP_NAME, true) 
{
	vp.camera.pos.z += 1.f;
	vp.camera.pos.y = 2.f;
	vp.camera.pivot.y -= 0.5f;
	vp.camera.updateViewMtx();
	parceBvh(&g_body, &g_motion, "assets/bvhs/smoothWalk.bvh");
}
AppBvhParsor::~AppBvhParsor()
{
	delete g_body;
	delete g_motion;
}
void AppBvhParsor::canvasUpdate()
{
	if( !is_paused ) {
		elapsed_time += delta_time;
	}
	static int prev_frame = -1;
	// elapsed_time += delta_time;
	int frameIdx = int(elapsed_time / g_motion->frame_dt);
	frameIdx = frameIdx % g_motion->nr_frames;

	if( frameIdx == prev_frame )
		return;
	prev_frame = frameIdx;
	g_body->setPose(g_motion->frames[frameIdx]);
}
void AppBvhParsor::canvasDraw() const
{
	g_body->draw(*this);

	// axis object 10cm
	constexpr vec3 ori{0,0,0};
	drawCylinder(ori, ori+vec3{0.1f,0,0}, 0.025f, {1,0,0});
	drawCylinder(ori, ori+vec3{0,0.1f,0}, 0.025f, {0,1,0});
	drawCylinder(ori, ori+vec3{0,0,0.1f}, 0.025f, {0,0,1});

	drawQuad({0,0,0}, {0,1,0}, vec2{100,100}, {0,0.3f,0});
}

void AppBvhParsor::canvasImGui()
{
	ImGui::Begin("test window##ik");
	ImGui::Checkbox("pause", &is_paused);
	if( is_paused ) {
		if( ImGui::Button("next") ) {
			elapsed_time += g_motion->frame_dt;
		}
	}

	
	ImGui::End();
}

void AppBvhParsor::dndCallback(int count, const char** paths)
{
	delete g_body;
	delete g_motion;
	parceBvh(&g_body, &g_motion, paths[count-1]);
	elapsed_time = 0.f;
}