#include "app_particle.h"
#include <limbrary/log.h>
#include <limbrary/asset_lib.h>
#include <limbrary/limgui.h>

using namespace glm;
using namespace std;
using namespace lim;

#include "cloth_coef.h"

/*
	1unit : m, kg, sec
*/
namespace {
	AppParticle* g_app = nullptr;
}


struct Particle {
	vec3 p = vec3{0};
	vec3 v = vec3{0};
	vec3 f = vec3{0};

	vec3 color = {1.f,1.f,0.f};
	bool fixed = false;

	void clearForce() {
		f = vec3(0);
	}
	void addForce(const vec3& force) {
		f += force;
	}
	void updateVel(float dt) {
		if( fixed ) return;
		vec3 acc = f/ptcl_mass;
		v += acc*dt;
	}
	void updatePos(float dt) {
		p += v*dt; // p먼저 업데이트해야 발산안하는 이유가 뭐지
	}
	void draw() {
		g_app->drawSphere(p, (fixed)?vec3{1,0,0}:color);
	}
};

struct Spring {
	Particle& p1;
	Particle& p2;
	float oriLength;
	vec3 color = {1.f,0.8f,0.f};

	Spring(Particle& p1, Particle& p2) 
		: p1(p1), p2(p2) {
		setLength();
	}
	void setLength() {
		oriLength = glm::length(p1.p-p2.p);
	}
	void applyForce(float ks, float kd)
	{
		vec3 diffP = p2.p - p1.p;
		vec3 diffV = p2.v - p1.v;

		float curLength = glm::length(diffP);
		vec3 dir = diffP/curLength;
		float diffL = curLength - oriLength;
		
		// force : p1이 당겨지는 힘
		float force = ks*diffL/oriLength;
		force += kd * dot(diffV, dir)*oriLength;

		p1.addForce( force * dir);
		p2.addForce(-force * dir);
	}
	void draw() {
		g_app->drawCylinder(p1.p, p2.p, color);
	}
};

struct ICollider {
	vec3 p = vec3{0};
	vec3 old_p = vec3{0};
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
			ptcl.v = pvt + Kr*pvn;
			ptcl.addForce(-Kmu*pvt);
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
			ptcl.v = pvt + Kr*pvn;
			ptcl.addForce(-Kmu*pvt);
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



struct Cloth {
	vector<Particle> ptcls;
	vector<Spring> stretch_sprs;
	vector<Spring> shear_sprs;
	vector<Spring> bending_sprs;


	inline int idxP(int x, int y) {
		return x+nr_p.x*y;
	}
	void resize() {
		inter_p_size = cloth_size/vec2{nr_p.x-1, nr_p.y-1};
		ptcl_mass = cloth_mass/(nr_p.x*nr_p.y);

		ptcls.clear();
		ptcls.reserve(nr_p.x*nr_p.y);
		for(int x=0; x<nr_p.x; x++) for(int y=0; y<nr_p.y; y++) {
			ptcls.emplace_back();
		}

		// stretch
		int nrSprs = (nr_p.x-1)*(nr_p.y) + (nr_p.x)*(nr_p.y-1);
		stretch_sprs.clear();
		stretch_sprs.reserve(nrSprs);
		for(int x=0; x<nr_p.x-1; x++) for(int y=0; y<nr_p.y; y++) {
			stretch_sprs.emplace_back(ptcls[idxP(x, y)], ptcls[idxP(x+1,y)]);
		}
		for(int x=0; x<nr_p.x; x++) for(int y=0; y<nr_p.y-1; y++) {
			stretch_sprs.emplace_back(ptcls[idxP(x, y)], ptcls[idxP(x,y+1)]);
		}
		
		// shear
		nrSprs = (nr_p.x-1)*(nr_p.y-1) + (nr_p.x-1)*(nr_p.y-1);
		shear_sprs.clear();
		shear_sprs.reserve(nrSprs);
		for(int x=0; x<nr_p.x-1; x++) for(int y=0; y<nr_p.y-1; y++) {
			shear_sprs.emplace_back(ptcls[idxP(x,  y)], ptcls[idxP(x+1,y+1)]);
			shear_sprs.emplace_back(ptcls[idxP(x+1,y)], ptcls[idxP(x,  y+1)]);
		}

		// bending
		nrSprs = glm::max(0, (nr_p.x-2)*(nr_p.y) + (nr_p.x)*(nr_p.y-2));
		bending_sprs.clear();
		bending_sprs.reserve(nrSprs);
		for(int x=0; x<nr_p.x-2; x++) for(int y=0; y<nr_p.y; y++) {
			bending_sprs.emplace_back(ptcls[idxP(x, y)], ptcls[idxP(x+2,y)]);
		}
		for(int x=0; x<nr_p.x; x++) for(int y=0; y<nr_p.y-2; y++){
			bending_sprs.emplace_back(ptcls[idxP(x, y)], ptcls[idxP(x,y+2)]);
		}

		Transform ctf;
		ctf.pos = {0,2,0};
		ctf.ori = quat(rotate(H_PI*0.0f, vec3{1,0,0}));
		ctf.scale = vec3(cloth_size.x, 1, cloth_size.y);
		ctf.scale *= 0.5f;
		ctf.update();

		vec2 startUv = {-1.f, -1.f};
		vec2 stepSize = vec2{2.f}/vec2{nr_p.x-1, nr_p.y-1};
		for(int x=0; x<nr_p.x; x++) for(int y=0; y<nr_p.y; y++) {
			vec2 uv = startUv + stepSize*vec2(x,y);
			vec3 wPos = vec3(ctf.mtx*vec4(uv.x, 0, uv.y, 1));
			Particle& p = ptcls[idxP(x,y)];
			p.p = wPos;
			p.v = vec3(0.f);
		}
		ptcls[0].fixed = true;
		for(auto& spr: stretch_sprs) spr.setLength();
		for(auto& spr: shear_sprs) 	 spr.setLength();
		for(auto& spr: bending_sprs) spr.setLength();


		ptcls[0].fixed = true;
		ptcls[nr_p.x-1].fixed = true;
	}
	void update(float dt, vector<ICollider*>& colliders) {
		for(auto& p : ptcls) {
			p.clearForce();
			p.addForce(-(Ka*p.v*ptcl_mass)/cloth_mass); // 공기저항
			p.addForce(G*ptcl_mass); // 자유낙하법칙
		}
		for(auto& s : stretch_sprs) s.applyForce(stretch_pct*Ks, Kd);
		for(auto& s : shear_sprs)   s.applyForce(shear_pct*Ks,   Kd);
		for(auto& s : bending_sprs) s.applyForce(bending_pct*Ks, Kd);
		for(auto& p : ptcls) p.updateVel(dt);
		for(auto& c : colliders) c->applyCollision(ptcls);
		for(auto& p : ptcls) p.updatePos(dt);
	}
	void draw() {
		for(auto& s : stretch_sprs) s.draw();
		for(auto& s : shear_sprs)   s.draw();
		for(auto& s : bending_sprs) s.draw();
		for(auto& p : ptcls) p.draw();
	}
};


namespace {
	vector<Particle> particles;
	Cloth cloth;
	vector<ICollider*> colliders;
	vector<ColAnimator> animators;
	Particle* picked_ptcl = nullptr;
}

static void initScene() {
	colliders.reserve(5);
	particles.reserve(100);
	animators.reserve(5);
	
	// p0
	particles.emplace_back();

	ColPlane* plane = new ColPlane();
	colliders.push_back(plane);

	// ColSphere* sphere = new ColSphere();
	// sphere->p = {-0.2, 0, 0.3};
	// colliders.push_back(sphere);

	// vector<ColAnimator::KeyPos> keys =  {
	// 	{0.f, vec3(0,0,0)},
	// 	{1.f, vec3(2,0,0)},
	// 	{2.f, vec3(1,0,0)},
	// 	{3.f, vec3(2,0,0)},
	// 	{4.f, vec3(0,0,0)},
	// };
	// animators.emplace_back(sphere, move(keys));
}
static void deinitScene() {
	for( auto* p : colliders ) {
		delete p;
	}
	colliders.clear();
	particles.clear();
	animators.clear();
}

static void resetScene() {
	Particle& ptcl = particles[0];
	ptcl.p = vec3(1, 2, 0);
	ptcl.v = {1.1f,0,0};

	cloth.resize();
}
static void resetParams() {
	time_speed = def_time_speed;
	step_size = def_step_size;
	Ka = def_Ka; 	
	Kr = def_Kr;
	Kmu = def_Kmu;
	Ks = def_Ks; 		
	Kd = def_Kd;
	
	is_pause = true;
	ptcl_mass = def_cloth_m;
	stretch_pct = def_stretch_pct;
	shear_pct = def_shear_pct;
	bending_pct = def_bending_pct;
}

AppParticle::AppParticle() : AppBaseCanvas3d(1200, 780, APP_NAME, false)
{
	g_app = this;	
	initScene();
	resetParams();
	resetScene();
}
AppParticle::~AppParticle()
{
	deinitScene();
}
void AppParticle::canvasUpdate() 
{
	float dt = (is_pause)? 0 : delta_time*time_speed;
	for(auto& a : animators) a.animate(dt);
	dt /= float(step_size);
	for(int i=0; i<step_size; i++)
	{
		for(auto& p : particles) {
			p.clearForce();
			p.addForce(-(Ka*p.v*ptcl_mass)/cloth_mass); // 공기저항
			p.addForce(G*ptcl_mass); // 자유낙하법칙
			p.updateVel(dt);
		}
		for(auto& c : colliders) c->applyCollision(particles);
		for(auto& p : particles) p.updatePos(dt);

		cloth.update(dt, colliders);	
	}
}
void AppParticle::canvasDraw() const {
	for(auto& c : colliders) c->draw();
	for(auto& p : particles) p.draw();
	cloth.draw();

	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});
}

static void pickClosestPtclInRay(vec3 ray, vec3 rayO, vector<Particle>& ps) {
	float minDepth = FLT_MAX;
	for( Particle& p : ps ) {
		vec3 toObj = p.p - rayO;
		float distFromLine = glm::length( glm::cross(ray, toObj) );
        float distProjLine = glm::dot(ray, toObj);

        if( distFromLine < 0.02f ) {
            if( distProjLine>0 && minDepth>distProjLine ) {
				minDepth = distProjLine;
				picked_ptcl = &p;
            }
		}
	}
	if( picked_ptcl ) {
		picked_ptcl->fixed = !picked_ptcl->fixed;
		picked_ptcl->v = vec3(0);
	}
}

void AppParticle::canvasImGui()
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
	ImGui::SliderInt("step size", &step_size, 1, 60);

	ImGui::SliderFloat("plane bounce damping", &Kr, 0.01f, 1.f);
	ImGui::SliderFloat("plane friction", &Kmu, 0.01f, 1.f);

	ImGui::SliderFloat("air damping", &Ka, 0.0001f, 0.9f, "%.5f");

	if(ImGui::SliderFloat("cloth mass", &cloth_mass, 0.05f, 0.7f)) {
		ptcl_mass = cloth_mass/(nr_p.x*nr_p.y);
	}
	ImGui::SliderFloat("spring coef", &Ks, 10.f, 70.f);
	ImGui::SliderFloat("spring damping coef", &Kd, 0.00f, 1.4f);
	ImGui::SliderFloat("stretch", &stretch_pct, 0.1f, 1.f);
	ImGui::SliderFloat("shear", &shear_pct, 0.1f, 1.f);
	ImGui::SliderFloat("bending", &bending_pct, 0.1f, 1.f);

	// if(ImGui::SliderInt2("nr cloth ptcls", (int*)&nr_p, 2, 200)) {
	// 	makeClothData();
	// }
	if(ImGui::SliderInt("nr cloth ptcls", (int*)&nr_p.x, 2, 200)) {
		nr_p.y = nr_p.x;
		cloth.resize();
	}
	ImGui::End();


	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickClosestPtclInRay(mouseRay, vp.camera.pos, particles);
		pickClosestPtclInRay(mouseRay, vp.camera.pos, cloth.ptcls);
	} else if(picked_ptcl && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		vec3& dstP = picked_ptcl->p;
		const vec3 toObj = dstP-vp.camera.pos;
		const vec3 mouseRay = vp.getMousePosRayDir();
		const float depth = dot(vp.camera.front, toObj)/dot(vp.camera.front, mouseRay);
		picked_ptcl->p = depth*mouseRay+vp.camera.pos;
		picked_ptcl->v = vec3(0);
	} else if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		picked_ptcl = nullptr;
	}

	if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
		particles.emplace_back();
		Particle& p = particles.back();
		p.p = vp.camera.pos;
		p.v = vp.getMousePosRayDir()*20.f;
	}
}