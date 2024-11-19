/*
	imdongye@naver.com
	fst: 2024-07-23
	lst: 2024-07-23
Note:
	glim = glm + lim
*/

#ifndef __glm_tools_h_
#define __glm_tools_h_

#include "log.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
// #include <glm/gtc/random.hpp>
// #include <glm/gtx/intersect.hpp>

namespace lim {
    namespace log {
		inline void pure(const glm::vec2& v) {
			log::pure("%-3.3f %-3.3f\n", v.x, v.y);
		}
		inline void pure(const glm::vec3& v) {
			log::pure("%-3.3f %-3.3f %-3.3f\n", v.x, v.y, v.z);
		}
		inline void pure(const glm::vec4& v) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", v.x, v.y, v.z, v.w);
		}
		inline void pure(const glm::quat& q) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", q.x, q.y, q.z, q.w);
		}
		inline void pure(const glm::mat3& m) {
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[0][0], m[0][1], m[0][2]);
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[1][0], m[1][1], m[1][2]);
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[2][0], m[2][1], m[2][2]);
		}
		inline void pure(const glm::mat4& m) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
		}
	}
}

namespace glim {
	constexpr float pi45 = 0.78539816339f; // quarter
	constexpr float pi90 = 1.57079632679f; // half
	constexpr float pi 	 = 3.14159265359f; // float
	constexpr float pi2  = 6.28318530718f;
	constexpr float feps = std::numeric_limits<float>::epsilon();
	constexpr glm::vec3 maximum_vec3 = glm::vec3(std::numeric_limits<float>::max());
	constexpr glm::vec3 minimum_vec3 = glm::vec3(std::numeric_limits<float>::min());
	constexpr glm::uint maximum_uint = (glm::uint)-1;
	constexpr glm::vec3 up = {0,1,0};
	constexpr glm::vec3 right = {1,0,0};
	constexpr glm::vec3 front = {0,0,1};

	inline float randFloat() {
  		return (float)std::rand() / RAND_MAX;
	}

	// From: https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
	inline int fastIntCeil(int x, int y) {
		return (x % y) ? (x / y + 1) : (x / y);
	}

    // glm::exp(quat) fixed version
	inline glm::quat exp(const glm::quat& q) {
		const glm::vec3 u{q.x, q.y, q.z};
		const float Angle = glm::length(u);
		if( Angle < glm::epsilon<float>() ) {
			return {glm::cos(Angle), glm::sin(Angle)*u};
		}
		const glm::vec3 v{u/Angle};
		return {glm::cos(Angle), glm::sin(Angle)*v};
	}


	inline glm::quat rotateV(const glm::vec3& axis, const float deg) {
		float rad = glm::radians(deg); // 2*PI*deg/360.f;
		glm::quat log_q = glm::quat(0, axis * rad * 0.5f);
		return exp(log_q);
	}


    inline glm::vec3 triNormal(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        return glm::normalize( glm::cross(p2-p1, p3-p1) );
    }

	inline float cotInterVec(const glm::vec3& a, const glm::vec3& b) {
		return glm::dot(a, b) / glm::length(glm::cross(a, b));
	}
	
	inline float acosApprox(float x) {
		// See: https://www.johndcook.com/blog/2022/09/06/inverse-cosine-near-1/
		float abs_x = glm::min(glm::abs(x), 1.0f); // Ensure that we don't get a value larger than 1
		float val = glm::sqrt(1.0f - abs_x) * (pi90 - 0.175394f*abs_x);
		return x<0 ? pi-val : val;
	}


	// From: https://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	inline float signedTetrahedronVolumeTimesSix(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
        // return glm::dot(v1, glm::cross(v2, v3))/6.f;
        return glm::dot(v1, glm::cross(v2, v3));
    }

	inline bool isZeroNear(float z) {
		return glm::abs(z) < feps;
	}
	inline bool isZeroNear(const glm::vec2& z) {
		return glm::abs(z.x) < feps && glm::abs(z.y) < feps;
	}
	inline bool isZeroNear(const glm::vec3& z) {
		return glm::abs(z.x) < feps && glm::abs(z.y) < feps && glm::abs(z.z) < feps;
	}
	inline bool isNotZero(float z) {
		return z!=0.f;
	}
	inline bool isNotZero(const glm::vec2& z) {
		return z.x!=0.f || z.y!=0.f;
	}
	inline bool isNotZero(const glm::vec3& z) {
		return z.x!=0.f || z.y!=0.f || z.z!=0.f;
	}


	inline float inv(float a) {
		return 1.f / a;
	}

	inline glm::mat4 rotateX(float rad) {
		return {
			{1, 	   0,        0, 0},
			{0, cos(rad),-sin(rad), 0},
			{0, sin(rad), cos(rad), 0},
			{0, 	   0,        0, 1},
		};
	}

	inline glm::mat4 rotateY(float rad) {
		return {
			{ cos(rad), 0, sin(rad), 0},
			{ 0, 	   1,        0, 0},
			{-sin(rad), 0, cos(rad), 0},
			{ 0, 	   0,        0, 1},
		};
	}

	inline glm::mat4 rotateZ(float rad) {
		return {
			{ cos(rad), sin(rad), 0, 0}, 
			{-sin(rad), cos(rad), 0, 0}, 
			{        0,        0, 1, 0}, 
			{        0,        0, 0, 1}, 
		};
	}

/*
Ref:
	https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	https://en.wikipedia.org/wiki/Pl%C3%BCcker_coordinates
	https://members.loria.fr/SLazard/ARC-Visi3D/Pant-project/files/Line_Segment_Triangle.html
	https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/raytri/
	https://github.com/g-truc/glm/blob/ea678faff9340ae4a79f50f2edd947141405e128/glm/gtx/intersect.inl
	https://wjdgh283.tistory.com/entry/%EC%82%BC%EA%B0%81%ED%98%95-%EC%95%88%EC%97%90-%EC%9E%88%EB%8A%94-%EC%A0%90%EC%9D%98-%EB%AC%B4%EA%B2%8C%EC%A4%91%EC%8B%AC-%EC%A2%8C%ED%91%9Cbarycentric-coordinate-%EA%B5%AC%ED%95%98%EA%B8%B0
From:
	BrunoLevy
	https://stackoverflow.com/questions/42740765/intersection-between-line-and-triangle-in-3d
Todo:
	version of back face intersect 
Note:
	if intersect return depth
	else return -1
*/
	inline float intersectTriAndRay (
		const glm::vec3& O, const glm::vec3& D,
		const glm::vec3& A, const glm::vec3& B, const glm::vec3& C
	)
	{ 
		glm::vec3 E1, E2, N, AO, DAO;
		float det, invdet, u, v, t;
		E1 = B-A;
		E2 = C-A;
		N = glm::cross(E1,E2);
		det = -glm::dot(D, N);
		invdet = 1.f/det;
		AO  = O - A;
		DAO = glm::cross(AO, D);
		u =  glm::dot(E2,DAO) * invdet;
		v = -glm::dot(E1,DAO) * invdet;
		t =  glm::dot(AO,N)  * invdet; 
		if(det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0)
			return t;
		return -1.f;
	}
	inline float intersectTriAndRayBothFaces (
		const glm::vec3& O, const glm::vec3& D,
		const glm::vec3& A, const glm::vec3& B, const glm::vec3& C
	)
	{ 
		glm::vec3 E1, E2, N, AO, DAO;
		float det, invdet, u, v, t;
		E1 = B-A;
		E2 = C-A;
		N = glm::cross(E1,E2);
		det = -glm::dot(D, N);
		invdet = 1.f/det;
		AO  = O - A;
		DAO = glm::cross(AO, D);
		u =  glm::dot(E2,DAO) * invdet;
		v = -glm::dot(E1,DAO) * invdet;
		t =  glm::dot(AO,N)  * invdet; 
		if(t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0)
			return t;
		return -1.f;
	}
}
#endif