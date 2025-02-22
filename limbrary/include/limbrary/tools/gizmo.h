/*
	imdongye@naver.com
	fst: 2024-11-10
	lst: 2024-11-11

Note:
	draw function must called after draw scene

Todo:
	alpha draw for behind obj
    최적화: 변환행렬케싱후
    마지막에 한번에 인스턴스드로잉
*/

#ifndef __lim_gizmo_h_
#define __lim_gizmo_h_

#include <glm/glm.hpp>
#include <limbrary/3d/camera.h>

namespace lim
{
    namespace gizmo
    {
	void init();
	void deinit();
	void drawPoint(const glm::vec3& wPos, const glm::vec4& col, float scale, const Camera& cam, bool screenScaling = true);
	void drawArrow(const glm::vec3& wPos, const glm::vec3 wDir, const glm::vec4& col, float length, float width, const Camera& cam, bool screenScaling = true); // w world, s screen
    }
}

#endif