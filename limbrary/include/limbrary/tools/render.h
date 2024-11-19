/*
	imdongye@naver.com
	fst: 2024-09-04
	lst: 2024-09-04
*/

#ifndef __render_h_
#define __render_h_

#include "../texture.h"

namespace lim
{
    void drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj);
}


#endif