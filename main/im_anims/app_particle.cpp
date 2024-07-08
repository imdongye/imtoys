#include "app_particle.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/asset_lib.h>
#include <limbrary/limgui.h>

using namespace glm;
using namespace std;
using namespace lim;

/*
	1unit : m, kg, sec
*/
struct ICollider;
struct Particle;
struct Spring;
struct Cloth;
struct ColAnimator;

namespace {
	AppParticle* g_app = nullptr;

	constexpr float def_Ka = 0.026f;	// vicous drag 공기저항
	constexpr float def_Kmu = 0.1f; 	// 마찰 계수
	constexpr float def_Kr = 0.8f;		// 반발 계수
	constexpr float def_Ks = 30.f;		// 스프링 계수
	constexpr float def_Kd = 0.4f;		// 스프링 저항
	constexpr float def_collision_eps = 0.001f; // (particle ratius)
	constexpr float def_stretch_pct = 1.f;
	constexpr float def_shear_pct = 0.9f;
	constexpr float def_bending_pct = 0.75f;
	constexpr float def_M = 0.01f; // 10g 질량

	float time_speed = 1.f;
	int step_size = 3;
	float Ka = def_Ka; 	
	float Ks = def_Ks; 		
	float Kd = def_Kd;
	
	bool is_pause = true;
	float cloth_p_m = def_M;
	float stretch_pct = def_stretch_pct;
	float shear_pct = def_shear_pct;
	float bending_pct = def_bending_pct;

	const vec3 G = {0, -9.8, 0};

	
	vector<ICollider*> colliders;
	vector<Particle> particles;
	vector<Spring> springs;
	vector<Cloth> cloths;
	vector<ColAnimator> animators;

	Particle* picked_ptcl = nullptr;
}



struct Particle {
	vec3 p = vec3{0};
	vec3 v = vec3{0};
	vec3 f = vec3{0};
	float m = def_M;

	vec3 color = {1.f,1.f,0.f};
	bool fixed = false;

	void clearForce() {
		f = vec3(0);
	}
	void addForce(const vec3& force) {
		f += force;
	}
	void integrate(float dt) {
		if( fixed ) return;
		p += v*dt;
		v += f/m*dt;
	}
	void draw() {
		g_app->drawSphere(p, (fixed)?vec3{1,0,0}:color);
	}
};

struct Spring {
	Particle& p1;
	Particle& p2;
	float ks_pct;
	float length;
	vec3 color = {1.f,0.8f,0.f};


	Spring(Particle& p1, Particle& p2, float ksPct=1.f) 
		: p1(p1), p2(p2), ks_pct(ksPct) {
		setLength();
	}
	void setLength() {
		length = glm::length(p1.p-p2.p);
	}
	void applyForce() {
		vec3 diffP = p2.p - p1.p;
		vec3 diffV = p2.v - p1.v;

		float curR = glm::length(diffP);
		vec3 dir = diffP/curR;
		
		// force : p1이 당겨지는 힘
		float force = ks_pct*Ks*(curR-length);
		force += Kd * dot(diffV, dir);

		p1.addForce( force * dir);
		p2.addForce(-force * dir);
	}
	void draw() {
		g_app->drawCylinder(p1.p, p2.p, color);
	}
};

struct Cloth {
	int nr_width, nr_height;
	vector<Particle*> p_ptcls;
	vector<Spring*> p_sprs;

	inline int idxP(int x, int y) {
		return x+nr_width*y;
	}
	Cloth(int nrWidth=10, int nrHeight=10)
		: nr_width(nrWidth), nr_height(nrHeight)
	{
		p_ptcls.reserve(nr_width*nr_height);
		for(int x=0; x<nr_width; x++) for(int y=0; y<nr_height; y++) {
			particles.emplace_back();
			p_ptcls.push_back(&particles.back());
		}

		int nr_stretch 	= (nr_width-1)*(nr_height)   + (nr_width)  *(nr_height-1);
		int nr_shear 	= (nr_width-1)*(nr_height-1) + (nr_width-1)*(nr_height-1);
		int nr_bending 	= glm::max(0, (nr_width-2)*(nr_height) + (nr_width)*(nr_height-2));
		p_sprs.reserve(nr_stretch + nr_shear + nr_bending);

		// stretch
		for(int x=0; x<nr_width-1; x++) for(int y=0; y<nr_height; y++) {
			springs.emplace_back(*p_ptcls[idxP(x, y)], *p_ptcls[idxP(x+1,y)], stretch_pct);
			p_sprs.push_back(&springs.back());
		}
		for(int x=0; x<nr_width; x++) for(int y=0; y<nr_height-1; y++) {
			springs.emplace_back(*p_ptcls[idxP(x, y)], *p_ptcls[idxP(x,y+1)], stretch_pct);
			p_sprs.push_back(&springs.back());
		}
		
		// shear
		for(int x=0; x<nr_width-1; x++) for(int y=0; y<nr_height-1; y++) {
			springs.emplace_back(*p_ptcls[idxP(x, y)], *p_ptcls[idxP(x+1,y+1)], shear_pct);
			p_sprs.push_back(&springs.back());
			springs.emplace_back(*p_ptcls[idxP(x+1, y)], *p_ptcls[idxP(x,y+1)], shear_pct);
			p_sprs.push_back(&springs.back());
		}

		// bending
		for(int x=0; x<nr_width-2; x++) for(int y=0; y<nr_height; y++) {
			springs.emplace_back(*p_ptcls[idxP(x, y)], *p_ptcls[idxP(x+2,y)], bending_pct);
			p_sprs.push_back(&springs.back());

		}
		for(int x=0; x<nr_width; x++) for(int y=0; y<nr_height-2; y++){
			springs.emplace_back(*p_ptcls[idxP(x, y)], *p_ptcls[idxP(x,y+2)], bending_pct);
			p_sprs.push_back(&springs.back());
		}
	}
	void setPose(const mat4& tf) {
		vec2 startUv = {-1.f, -1.f};
		// 2m 2m
		vec2 stepUv = {2.f/(nr_width-1), 2.f/(nr_height-1)};
		for(int x=0; x<nr_width; x++) for(int y=0; y<nr_height; y++) {
			vec2 uv = startUv + stepUv*vec2(x,y);
			vec3 wPos = vec3(tf * vec4(uv.x, 0, uv.y, 1));
			Particle& p = *p_ptcls[idxP(x,y)];
			p.p = wPos;
			p.v = vec3(0.f);
			p.m = cloth_p_m;
		}
		for(Spring* s : p_sprs) {
			s->setLength();
		}
	}
};




struct ICollider {
	vec3 p = vec3{0};
	vec3 old_p = vec3{0};
	float k_r = def_Kr;
	float k_mu = def_Kmu;
	vec3 color = vec3(1.f);
	virtual void applyCollision(vector<Particle>& ptcls) const = 0;
	virtual void draw() = 0;
};

struct ColPlane : ICollider {
	vec3 n = {0,1,0};

	ColPlane() {
		color = {0,0.3f,0};
	}
	virtual void applyCollision(vector<Particle>& ptcls) const final {
		for( Particle& ptcl : ptcls ) {
			vec3 diffP = ptcl.p - p;
			if( dot(diffP, n) > def_collision_eps ) continue;
			if( dot(ptcl.v, n) > 0.f ) continue;

			vec3 pv = ptcl.v;
			vec3 pvn = -dot(ptcl.v, n) * n;
			vec3 pvt = pv + pvn;
			ptcl.v = pvt + k_r*pvn;
			ptcl.addForce(-k_mu*pvt);
		}

	}
	virtual void draw() override {
		g_app->drawQuad(p, n, color);
	}
};

struct ColSphere : ICollider {
	float r = 0.4f;
	vec4 color = vec4{1.f,0,0,1};


	virtual void applyCollision(vector<Particle>& ptcls) const final {
		for( Particle& ptcl : ptcls ) {
			vec3 diffP = ptcl.p - p;
			float curR = length(diffP);
			vec3 n = diffP/curR;
			if( curR > r+def_collision_eps ) continue;
			if( dot(ptcl.v, n) > 0.f ) continue;

			vec3 pv = ptcl.v;
			vec3 pvn = -dot(pv, n) * n;
			vec3 pvt = pv + pvn;
			ptcl.v = pvt + k_r*pvn;
			ptcl.addForce(-k_mu*pvt);
		}
	}
	virtual void draw() override {
		g_app->drawSphere(p, color, r);
	}
};



struct ColAnimator {
	struct KeyPos {
		float t;
		vec3 pos;
	};
	vector<KeyPos> keys;
	ICollider* col;
	float time = 0;
	float time_length;
	ColAnimator() = delete;
	ColAnimator(ICollider* c, vector<KeyPos>&& ks)
		:col(c), keys(move(ks)) {
		time_length = keys.back().t;
	}
	void animate(const float dt) {
		const int nrKeys = keys.size();
		vec3 newPos{0,0,0};
		for(int i=0; i<nrKeys; i++) {
			if( time < keys[i].t ) {
				KeyPos& k2 = keys[i];
				KeyPos& k1 = keys[i-1];
				float dt = k2.t - k1.t;
				float diff = time-k1.t;
				float factor = diff/dt;
				newPos = factor*(k2.pos-k1.pos) + k1.pos;
				break;
			}
		}
		col->p = newPos;

		time += dt;
		if(time>time_length) {
			time -= time_length;
		}
	}
};


static void initScene() {
	colliders.reserve(5);
	particles.reserve(100+25*25);
	springs.reserve(25*25*6);
	animators.reserve(5);
	
	// p0
	particles.emplace_back();

	cloths.push_back(Cloth(7, 7));

	ColPlane* plane = new ColPlane();
	colliders.push_back(plane);

	ColSphere* sphere = new ColSphere();
	sphere->p = {-0.2, 0, 0.3};
	colliders.push_back(sphere);

	vector<ColAnimator::KeyPos> keys =  {
		{0.f, vec3(0,0,0)},
		{1.f, vec3(2,0,0)},
		{2.f, vec3(1,0,0)},
		{3.f, vec3(2,0,0)},
		{4.f, vec3(0,0,0)},
	};
	animators.emplace_back(sphere, move(keys));
}
static void deinitScene() {
	for( auto* p : colliders ) {
		delete p;
	}
	colliders.clear();
	particles.clear();
	springs.clear();
	animators.clear();
}
static void resetScene() {
	Particle& ptcl = particles[0];
	ptcl.p = vec3(1, 2, 0);
	ptcl.v = {1.1f,0,0};

	//70cm
	Transform ctf;
	ctf.pos = {0,1,0};
	ctf.ori = quat(rotate(H_PI*0.8f, vec3{1,0,0}));
	ctf.scale = vec3(0.7f*0.5f);
	ctf.update();
	Cloth& cloth = cloths.back();
	cloth.setPose(ctf.mtx);
	cloth.p_ptcls[0]->fixed = true;
}
static void resetParams() {
	time_speed = 1.f;
	step_size = 3;
	particles[0].m = def_M;

	colliders[0]->k_r = def_Kr;
	colliders[0]->k_mu = def_Kmu;

	stretch_pct = def_stretch_pct;
	shear_pct = def_shear_pct;
	bending_pct = def_bending_pct;

	Ka = def_Ka;
	Kd = def_Kd;
	Ks = def_Ks;
}

AppParticle::AppParticle() : AppBaseCanvas3d(1200, 780, APP_NAME, false)
{
	g_app = this;	
	initScene();
	resetScene();
	resetParams();
}
AppParticle::~AppParticle()
{
	deinitScene();
}
void AppParticle::render() 
{
	float dt = (is_pause)? 0 : (delta_time*time_speed)/float(step_size);
	dt = glm::min(dt, 1.f/165.f);
	for(int i=0; i<step_size; i++)
	{
		for(auto& a : animators) a.animate(dt);
		for(auto& p : particles) p.clearForce();
		for(auto& p : particles) p.addForce(-Ka*p.v); // 공기저항
		for(auto& p : particles) p.addForce(G*p.m); // 자유낙하법칙
		for(auto& s : springs) 	 s.applyForce();
		for(auto& c : colliders) c->applyCollision(particles);
		for(auto& p : particles) p.integrate(dt);
		for(auto& c : colliders) c->draw();
		for(auto& p : particles) p.draw();
		for(auto& s : springs)   s.draw();
	}


	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});
}
void AppParticle::renderImGui()
{
	log::drawViewer("logger##particle");

	ImGui::Begin("test window##particle");
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	if(ImGui::Button("restart")) {
		resetScene();
	}
	if(ImGui::Button("reset params")) {
		resetParams();
	}
	ImGui::Checkbox("pause", &is_pause);


	ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	ImGui::SliderInt("step size", &step_size, 1, 10);
	ImGui::SliderFloat("p0 mass", &particles[0].m, 0.01f, 3.f);

	ImGui::SliderFloat("plane bounce damping", &colliders[0]->k_r, 0.01f, 1.f);
	ImGui::SliderFloat("plane friction", &colliders[0]->k_mu, 0.01f, 1.f);

	ImGui::SliderFloat("air damping", &Ka, 0.0001f, 0.09f, "%.5f");

	ImGui::SliderFloat("spring mass", &cloth_p_m, 0.001f, 0.1f);
	ImGui::SliderFloat("spring coef", &Ks, 10.f, 50.f);
	ImGui::SliderFloat("spring damping coef", &Kd, 0.00f, 1.f);

	ImGui::SliderFloat("stretch", &stretch_pct, 0.1f, 1.f);
	ImGui::SliderFloat("shear", &shear_pct, 0.1f, 1.f);
	ImGui::SliderFloat("bending", &bending_pct, 0.1f, 1.f);

	ImGui::End();
}

void AppParticle::mouseBtnCallback(int btn, int action, int mods) {
	if(btn==GLFW_MOUSE_BUTTON_1)
		return;
	if(btn==GLFW_MOUSE_BUTTON_3 && action==GLFW_PRESS) {
		particles.emplace_back();
		Particle& p = particles.back();
		p.p = vp.camera.pos;
		p.v = vp.getMousePosRayDir()*20.f;
	}
	if(action==GLFW_RELEASE) {
		picked_ptcl = nullptr;
		return;
	}
	const vec3 mouseRay = vp.getMousePosRayDir();

	float minDepth = FLT_MAX;
	int minDepthPtclIdx = -1;
	for(int i=0; i<particles.size(); i++) {
		Particle& p = particles[i];
		vec3 toObj = p.p - vp.camera.pos;
		float distFromLine = glm::length( glm::cross(mouseRay, toObj) );
        float distProjLine = glm::dot(mouseRay, toObj);

        if( distFromLine < 0.02f ) {
            if( distProjLine>0 && minDepth>distProjLine ) {
				minDepth = distProjLine;
				minDepthPtclIdx = i;
            }
		}
	}
	if( minDepthPtclIdx!=-1 ) {
		particles[minDepthPtclIdx].fixed = !particles[minDepthPtclIdx].fixed;
		if(particles[minDepthPtclIdx].fixed) {
			picked_ptcl = &particles[minDepthPtclIdx];
		}
	}
}

void AppParticle::cursorPosCallback(double xPos, double yPos) {
	if(!picked_ptcl)
		return;
	vec3& dstP = picked_ptcl->p;
	const vec3 toObj = dstP-vp.camera.pos;
	const vec3 mouseRay = vp.getMousePosRayDir();
	const float depth = dot(vp.camera.front, toObj)/dot(vp.camera.front, mouseRay);
	picked_ptcl->p = depth*mouseRay+vp.camera.pos;
	picked_ptcl->v = vec3(0);
}
