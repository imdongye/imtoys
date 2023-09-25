/*

2023-09-10 / imdongye



*/

#ifndef __renderer_h_
#define __renderer_h_

#include "../framebuffer.h"
#include "model.h"
#include "scene.h"

namespace lim
{
    void render( const Framebuffer& fb,
                 const Program& prog,
                 const Model& md );

    void render( const Framebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit );

    void render( const Framebuffer& fb,
                 const Camera& cam,
                 const Scene& scn );
}



#endif