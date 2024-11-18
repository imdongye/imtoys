/*
    2024-08-21 / imdongye

    skinned mesh for caching in gpu

Note!!
    individual only poss and nors
    other buffer are shared
*/

#ifndef __mesh_skinned_h_
#define __mesh_skinned_h_

#include "mesh.h"


namespace lim
{
    class MeshSkinned : public Mesh
    {
    public:
        const Mesh& src;
    public:
        MeshSkinned(const Mesh& _src);
        virtual ~MeshSkinned();
    };
}



#endif