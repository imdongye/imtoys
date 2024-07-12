#include <glm/glm.hpp>

namespace {
    constexpr float def_time_speed = 1.f;
	constexpr int 	def_step_size = 12;
	constexpr float def_Ka = 0.5f;	// vicous drag 공기저항
	constexpr float def_Kr = 0.8f;		// 반발 계수
	constexpr float def_Kmu = 0.1f; 	// 마찰 계수
	constexpr float def_Ks = 10.f;		// 스프링 계수
	constexpr float def_Kd = 0.2f;	// 스프링 저항
	constexpr float def_collision_eps = 0.001f; // (particle ratius)
	constexpr float def_stretch_pct = 1.f;
	constexpr float def_shear_pct = 0.9f;
	constexpr float def_bending_pct = 0.75f;
	constexpr float def_cloth_m = 0.200f; // 200g 질량
	constexpr glm::vec3 G = {0, -9.8, 0};

	glm::ivec2 nr_p{7, 7};
	glm::vec2 cloth_size{0.7, 0.7};
	glm::vec2 inter_p_size;

	float time_speed = def_time_speed;
	int step_size = def_step_size;
	float Ka = def_Ka; 	
	float Kr = def_Kr;
	float Kmu = def_Kmu;
	float Ks = def_Ks; 		
	float Kd = def_Kd;
	
	bool is_pause = true;
	float cloth_mass = def_cloth_m;
	float ptcl_mass;
	float stretch_pct = def_stretch_pct;
	float shear_pct = def_shear_pct;
	float bending_pct = def_bending_pct;
}