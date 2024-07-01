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

struct Particle {
	vec3 p, v, a;
	float m = 0.001f;
	vec4 color = vec4(1.f);
	bool fixed = false;

	void clearAcc() {
		a = vec3(0);
	}
	void addForce(const vec3& force) {
		a += force/m;
	}
	void integrate(float dt) {
		if( fixed ) return;
		p += v*dt;
		v += a*dt;
	}
	void draw() {
		g_app->drawSphere(p, 0.05f, color);
	}
};

struct ICollider {
	vec3 p, old_p;
	vec3 color = vec3(1.f);
	
	virtual void draw() = 0;
};

struct ColPlane : ICollider {
	vec3 p, n;
	vec4 color = vec4(1.f);
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
	for(auto& p : particles) {
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