/*
    2024-07-23 / im dong ye

	glim = glm + lim
*/

#ifndef __glm_tools_h_
#define __glm_tools_h_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/intersect.hpp>
#include <limits>


namespace glim {
	constexpr float pi45 = 0.78539816339f; // quarter
	constexpr float pi90 = 1.57079632679f; // half
	constexpr float pi 	 = 3.14159265359f; // float
	constexpr float pi2  = 6.28318530718f; // double
	constexpr float feps = std::numeric_limits<float>::epsilon();
	constexpr glm::vec3 maximum_vec3 = glm::vec3(std::numeric_limits<float>::max());
	constexpr glm::vec3 minimum_vec3 = glm::vec3(std::numeric_limits<float>::min());
	constexpr glm::uint maximum_uint = (glm::uint)-1;
	constexpr glm::vec3 up = {0,1,0};
	constexpr glm::vec3 right = {1,0,0};
	constexpr glm::vec3 front = {0,0,1};



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
		float val = glm::sqrt(1.0f - abs_x) * (pi / 2 - 0.175394f * abs_x);
		return x<0 ? pi-val : val;
	}


	// From: https://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	inline float signedTetrahedronVolumeTimesSix(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
        // return glm::dot(v1, glm::cross(v2, v3))/6.f;
        return glm::dot(v1, glm::cross(v2, v3));
    }


	inline bool isZero(float a) {
		return glm::epsilonEqual(a, 0.f, glm::epsilon<float>());
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

	// From: 
/*
	Ref:
	https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	https://en.wikipedia.org/wiki/Pl%C3%BCcker_coordinates
	https://members.loria.fr/SLazard/ARC-Visi3D/Pant-project/files/Line_Segment_Triangle.html
	https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/raytri/
	https://github.com/g-truc/glm/blob/ea678faff9340ae4a79f50f2edd947141405e128/glm/gtx/intersect.inl
	https://wjdgh283.tistory.com/entry/%EC%82%BC%EA%B0%81%ED%98%95-%EC%95%88%EC%97%90-%EC%9E%88%EB%8A%94-%EC%A0%90%EC%9D%98-%EB%AC%B4%EA%B2%8C%EC%A4%91%EC%8B%AC-%EC%A2%8C%ED%91%9Cbarycentric-coordinate-%EA%B5%AC%ED%95%98%EA%B8%B0
*/
	// barycentric coordinate 
	inline bool intersectTriAndSegLine(const glm::vec3& p1, const glm::vec3& p2
		, const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3
		, glm::vec3& point)
	{
		glm::vec3 diff, u, v, n, w0, w;
		float r, a, b;

		diff = p2 - p1;
		u = t2-t1;
		v = t3-t1;
		n = glm::cross(u, v);

		w0 = t1-p1;
		a = glm::dot(w0, n);
		b = glm::dot(diff, n);

		r = a/b;
		if( r<0.0 || r>1.0 )
			return false;
		
		point = p1 + r*diff;
		w = t1-point;

		float uu, uv, vv, wu, wv, D;
		uu = glm::dot(u,u);
		uv = glm::dot(u,v);
		vv = glm::dot(v,v);
		wu = glm::dot(w,u);
		wv = glm::dot(w,v);
		D = uv*uv - uu*vv;

		float s, t;
		s = (uv*wv - vv*wu) / D;
		if( s<0.0 || s>1.0 ) 
			return false;
		t = (uv*wu - uu*wv) / D;
		if( t<0.0 || t>1.0 ) 
			return false;

		return true;
	}


	// Todo: bool대신 깊이 반환
	inline bool intersectTriAndRay(const glm::vec3& origin, const glm::vec3& scaledDir
		, const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3
		, glm::vec3& point)
	{
		glm::vec3 u, v, n, w0, w;
		float r, a, b;

		u = t2-t1;
		v = t3-t1;
		n = glm::cross(u, v);

		// dot(w0, n) == 0
		// 
		w0 = t1-origin;
		a = glm::dot(w0, n);
		b = glm::dot(scaledDir, n);

		r = a/b;
		if( r<0.0 || r>1.0 )
			return false;
		
		point = origin + r*scaledDir;
		w = t1-point;

		float uu, uv, vv, wu, wv, D;
		uu = glm::dot(u,u);
		uv = glm::dot(u,v);
		vv = glm::dot(v,v);
		wu = glm::dot(w,u);
		wv = glm::dot(w,v);
		D = uv*uv - uu*vv;

		float s, t;
		s = (uv*wv - vv*wu) / D;
		if( s<0.0 || s>1.0 ) 
			return false;
		t = (uv*wu - uu*wv) / D;
		if( t<0.0 || t>1.0 ) 
			return false;

		return true;
	}

}


#endif