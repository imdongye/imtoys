/*
	imdongye
	fst 2024-11-10
	end 2024-11-11

	Note: draw function must called after draw scene
*/

#ifndef __lim_gizmo_h_
#define __lim_gizmo_h_

#include <glm/glm.hpp>
#include <limbrary/model_view/camera.h>

namespace lim
{
    namespace gizmo
    {
	void init();
	void deinit();
	void drawPoint(const glm::vec3& wPos, const glm::vec4& col, float scale, const Camera& cam, bool screenScaling = true);
	void drawArrow(const glm::vec3& wPos, const glm::vec3 wDir, const glm::vec4& col, float width, float scale, const Camera& cam, bool screenScaling = true); // w world, s screen
    }
}

#endif