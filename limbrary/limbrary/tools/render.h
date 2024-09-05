/*
    2024-09-04 imdongye

*/

#ifndef __render_h_
#define __render_h_

#include "../texture.h"

namespace lim
{
    void drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj);
}


#endif