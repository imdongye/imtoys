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

static AppParticle* g_app = nullptr;

/*
	1unit : m, kg, sec
*/

namespace {
	constexpr float def_Ka = 0.00025f;	// vicous drag 공기저항
	constexpr float def_Ks = 30.f;		// 스프링 계수
	constexpr float def_Kd = 0.15f;		// 스프링 저항
	constexpr float def_Kmu = 0.3f; 	// 마찰 계수
	constexpr float def_Kr = 0.3f;		// 반발 저항
	constexpr float def_collision_eps = 0.001f; // (particle ratius)
	constexpr float def_stretch_pct = 1.f;
	constexpr float def_shear_pct = 0.9f;
	constexpr float def_bending_pct = 0.75f;

	constexpr float def_particle_radius = 0.05f;
	constexpr float def_spring_radius = 0.02f;

	float Ka = def_Ka; 	
	float Ks = def_Ks; 		
	float Kd = def_Kd; 		
	
	float stretch_pct = def_stretch_pct;
	float shear_pct = def_shear_pct;
	float bending_pct = def_bending_pct;

	const vec3 G = {0, -9.8, 0};

	ivec2 cloth_particle_size = {20, 20};
	vec2 cloth_size = {1.f, 1.f}; 
}

struct Particle {
	vec3 p = vec3{0};
	vec3 v = vec3{0};
	vec3 f = vec3{0};
	float m = 0.1f;
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
		if(p.y<0)
			v = -v;
	}
	void draw() {
		g_app->drawSphere(p, def_particle_radius, color);
	}
};

struct Spring {
	Particle* p1;
	Particle* p2;
	float ks_pct;
	float r;
	vec3 color = {1.f,1.f,0.f};


	Spring(Particle* p1=nullptr, Particle* p2=nullptr, float ksPct=1.f) 
		: p1(p1), p2(p2)
		, ks_pct(ksPct)
		, r(length(p1->p - p2->p)) {
	}
	void applyForce() {
		vec3 diffP = p1->p - p2->p;
		float curR = length(diffP);
		float diffR = curR - r;
		vec3 dir = normalize(diffP);
		vec3 diffV = p1->v - p2->v;
		float force = ks_pct * Ks * diffR;
		force -= Kd * dot(diffV, dir);
		p1->addForce(-force * dir);
		p2->addForce( force * dir);
	}
	void draw() {
		g_app->drawCylinder(p1->p, p2->p, def_spring_radius, color);
	}
};

struct ICollider {
	vec3 p, old_p;
	vec3 color = vec3(1.f);
	virtual void applyCollision(vector<Particle>& particles) = 0;
	virtual void draw() = 0;
};

struct ColPlane : ICollider {
	vec3 p = vec3{0};
	vec3 n = {0,1,0};
	vec3 color = {1.f,0.f,0.f};
	float k_mu = def_Kmu;
	float k_r = def_Kr;

	virtual void applyCollision(vector<Particle>& particles) override {
		for( Particle& ptcl : particles ) {
			vec3 diffP = ptcl.p - p;
			if( dot(diffP, n) > def_collision_eps ) continue;
			if( dot(ptcl.v, n) > 0.f ) continue;

			vec3 pv = ptcl.v;
			vec3 pvn = -dot(ptcl.v, n) * n;
			vec3 pvt = pv + pvn;
			ptcl.v = pvt + k_r*pvn;
			ptcl.addForce(k_mu*pvt);
		}

	}
	virtual void draw() override {
		g_app->drawQuad(p, n, vec2(200.f), color);
	}
};

struct ColSphere : ICollider {
	vec3 p = vec3{0};
	float r = 1.f;
	vec4 color = vec4{1.f,0,0,1};
	float k_mu = def_Kmu;
	float k_r = def_Kr;

	virtual void applyCollision(vector<Particle>& particles) override {	
		for( Particle& ptcl : particles ) {
			vec3 diffP = ptcl.p - p;
			float curR = length(diffP);
			vec3 n = diffP/curR;
			if( curR > r+def_collision_eps ) continue;
			if( dot(ptcl.v, n) > 0.f ) continue;

			vec3 pv = ptcl.v;
			vec3 pvn = -dot(ptcl.v, n) * n;
			vec3 pvt = pv + pvn;
			ptcl.v = pvt + k_r*pvn;
			ptcl.addForce(k_mu*pvt);
		}
	}
	virtual void draw() override {
		g_app->drawSphere(p, r, color);
	}
};

struct Cloth {
	vector<Particle> particles;
	vector<Spring> springs;
	
	Cloth(): particles(400), springs(2400) {}
	void clear() {
		particles.clear();
		springs.clear();
	}
	void draw() {
		for(Particle& ptcl : particles) ptcl.draw();
		for(auto& spr : springs) spr.draw();
	}
	void update(float dt, vector<ICollider*>& colliders) {
		for(auto& p : particles) p.clearForce();
		for(auto& p : particles) p.addForce(-Ka * p.v);
		for(auto& p : particles) p.addForce(G);
		for(auto& s : springs) 	 s.applyForce();
		for(auto& c : colliders) c->applyCollision(particles);
		for(auto& p : particles) p.integrate(dt);
	}
};

static ColPlane col_plane;
static vector<Particle> particles;

lim::AppParticle::AppParticle() : AppBaseCanvas3d(1200, 780, APP_NAME, false)
{
	g_app = this;

	particles.reserve(1000);
	Particle p;
	p.p = vec3(0, 3, 0);
	particles.push_back(p);
}
lim::AppParticle::~AppParticle()
{
}
void lim::AppParticle::render() 
{
	col_plane.draw();
	// drawSphere(vec3{0}, 1, vec4(1,0,0,0));
	for(auto& p : particles) {
		p.clearForce();
		p.addForce(vec3{0,-1,0}*0.98f);
		p.integrate(delta_time);
		p.draw();
	}
}
void lim::AppParticle::renderImGui()
{
	log::drawViewer("logger##template");

	ImGui::Begin("test window##template");
	LimGui::PlotVal("delta time", "(ms)", delta_time*1000.f);
	ImGui::End();
}