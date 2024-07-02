#include "app_particle.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/asset_lib.h>

using namespace glm;
using namespace std;
using namespace lim;

static AppParticle* g_app = nullptr;

/*
	1unit : m, kg
*/

struct Particle {
	vec3 p = vec3{0};
	vec3 v = vec3{0};
	vec3 f = vec3{0};
	float m = 0.1f;
	vec4 color = vec4{1.f,1,0,0};
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
		g_app->drawSphere(p, 0.05f, color);
	}
};

struct Spring {
	Particle *p1, *p2;

};

struct ICollider {
	vec3 p, old_p;
	vec3 color = vec3(1.f);
	
	virtual void draw() = 0;
};

struct ColPlane : ICollider {
	vec3 p = vec3{0};
	vec3 n = {0,1,0};
	vec4 color = vec4{1.f,0,0,1};
	float k_mu, k_r;

	virtual void draw() override {
		g_app->drawQuad(p, n, vec2(200.f), color);
	}
};

static ColPlane col_plane;
static vector<Particle> particles;

lim::AppParticle::AppParticle() : Canvas3dApp(1200, 780, APP_NAME, false)
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
	ImGui::Text("delta time : %f", delta_time);
	ImGui::End();
}