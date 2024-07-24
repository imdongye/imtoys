/*
    2024-07-23 / im dong ye
*/

#ifndef __glm_tools_h_
#define __glm_tools_h_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace glim {
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
    

    inline glm::vec3 triNormal(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        return glm::normalize( glm::cross(p2-p1, p3-p1) );
    }


	// From: https://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
	inline float signedTetrahedronVolume(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
        return glm::dot(v1, glm::cross(v2, v3))/6.f;
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
}


#endif