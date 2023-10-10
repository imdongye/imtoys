/*

2023-09-10 / imdongye

Scene
renderer에서 사용하기위한 단순 컨테이너

Todo: 
1. 멀티라이트
2. 노드(메쉬들) 별 하이라키 transformation

*/

#ifndef __renderer_h_
#define __renderer_h_

#include <vector>
#include "model.h"
#include "light.h"
#include "../framebuffer.h"
#include "model.h"

namespace lim
{
	class Scene
	{
    private:
		std::vector<const Model*> my_mds;
        void deleteOwn();
    public:
		std::vector<const Model*> models;
		std::vector<const Light*> lights;
    private:
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
	public:
		Scene();
		Scene(Scene&& src) noexcept;
		Scene& operator=(Scene&& src) noexcept;
        ~Scene() noexcept;
        void addModel( const Model* md, bool deleteWhenScnDeleted = false );
        void subModel( const Model* md );
        void addLight( const Light* lit );
        void subLight( const Light* lit );
	};

    void render( const Framebuffer& fb,
                 const Program& prog,
                 const Model& md );

    // use prog in param
    void render( const Framebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit );

    // use prog in mat
    void render( const Framebuffer& fb,
                 const Camera& cam,
                 const Scene& scn );
}



#endif