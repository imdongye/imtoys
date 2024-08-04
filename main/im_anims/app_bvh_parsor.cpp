/*
	Todo: 
*/

#include "app_bvh_parsor.h"
#include <limbrary/glm_tools.h>
#include <limbrary/log.h>
#include <limbrary/limgui.h>
#include <fstream>
#include <filesystem>

using namespace glm;
using namespace std;
using namespace lim;


struct Motion {
	int nr_frames, nr_joints;
	float frame_dt;
	float running_time;
	// nrFrames * nrJoints * nrCannels
	vector<vector<vector<float>>> frames;
};



struct Joint {
	enum CHANNEL {
		X_POS, Y_POS, Z_POS,
		X_ROT, Y_ROT, Z_ROT,
	};
	int parent_id = -1;
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

	void update(const vec3& parentP, const quat& parentQ, float scale = 1) {
		quat l = quat(0, scale*link);
		quat p = parentQ * l * inverse(parentQ);
		position = parentP + vec3(p.x, p.y, p.z);
		orientation = parentQ * q;
	}
};




struct Body {
	int nr_joints;
	vector<Joint> joints;
	vec3 position;
	float scale = 1.f;

	void draw(const AppBvhParsor& app) {
		for (auto& j : joints) {
			vec3 p = j.position;
			app.drawSphere(p, {1,0.5f,0});
			if (j.parent_id >= 0) {
				vec3 pp = joints[j.parent_id].position;
				app.drawCylinder(p, pp, {1,0.7f,0});
			}
		}
	}


	void updateJointsGlobal() {
		joints[0].update(position, quat(1, 0, 0, 0));
		for (int i = 1; i < joints.size(); i++) {
			Joint& j = joints[i];
			const Joint& p = joints[j.parent_id];
			j.update(p.position, p.orientation, scale);
		}
	}


	void setPose(const vector<vector<float>>& frame) {
		for (int i = 0; i < nr_joints; i++) {
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



static void parceJoint(int parentId, bool isEnd, Body& body, ifstream& input)
{
	string token;
	body.joints.push_back({});
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

	int curId = body.nr_joints++;
	do {
		input >> token;
		if( token == "JOINT" )
			parceJoint(curId, false, body, input);
		else if( token == "End" )
			parceJoint(curId, true, body, input);
	} while (token != "}");
}

static void parceBvh(Body** ppBody, Motion** ppMotion, const char* path) {
	string token;

	*ppBody = new Body();
	*ppMotion = new Motion();

	Body& body = **ppBody;
	Motion& motion = **ppMotion;

	ifstream input(path);
	if (!input.is_open()) {
		fprintf(stderr, "Noinput file %s", path);
		return;
	}

	// parsing bone hierarchy ==============
	input >> token; // HIERARCHY
	input >> token; // ROOT
	parceJoint(-1, false, body, input);



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
			vector<float>& key = frame[j];
			key.resize(joint.nr_channels);
			for(int k = 0; k < joint.nr_channels; k++)
			{
				input >> key[k];
			}
		}
	}


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
}







namespace {
	Body* g_body;
	Motion* g_motion;
	float elapsed_time = 0.f;
}

AppBvhParsor::AppBvhParsor()
	: AppBaseCanvas3d(1200, 780, APP_NAME, true) 
{
	parceBvh(&g_body, &g_motion, "assets/bvhs/smoothWalk.bvh");
	g_body->scale = 0.5f;
}
AppBvhParsor::~AppBvhParsor()
{
	delete g_body;
	delete g_motion;
}
void AppBvhParsor::canvasUpdate()
{
	static int prev_frame = -1;
	// elapsed_time += delta_time;
	int frameIdx = elapsed_time / g_motion->frame_dt;
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
	constexpr vec3 ori{1,0,0};
	drawCylinder(ori, ori+vec3{0.1f,0,0}, {1,0,0});
	drawCylinder(ori, ori+vec3{0,0.1f,0}, {0,1,0});
	drawCylinder(ori, ori+vec3{0,0,0.1f}, {0,0,1});

	drawQuad({0,0,0}, {0,1,0}, {0,0.3f,0});
}

void AppBvhParsor::canvasImGui()
{
	log::drawViewer("logger##ik");

	ImGui::Begin("test window##ik");
	if( ImGui::Button("next") ) {
		elapsed_time += g_motion->frame_dt;
	}
	ImGui::End();
}

void AppBvhParsor::dndCallback(int count, const char **paths)
{

}