/*
	imdongye
	fst 2024-11-10
	end 2024-11-10
*/

#ifndef __lim_gizmo_h_
#define __lim_gizmo_h_

#include <glm/glm.hpp>

namespace lim
{
    namespace gizmo
    {
	void init();
	void drawArrow(const glm::vec3& wPos, const glm::vec3 wDir, const glm::vec4& col, const float sScale); // w world, s screen
	void destroy();
    }
}

#endif