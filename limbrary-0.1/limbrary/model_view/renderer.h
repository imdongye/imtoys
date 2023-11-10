/*

2023-09-10 / imdongye

Scene
renderer에서 사용하기위한 단순 컨테이너
my_mds에 넣어두면 해당씬에 생명주기가 종속되어 같이삭제됨.

Todo: 
1. 멀티라이트
2. 노드(메쉬들) 별 하이라키 transform

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
  	public:
		std::vector<Model*> my_mds;
        std::vector<Light*> my_lits;
		std::vector<const Model*> models;
		std::vector<const Light*> lights;
        const Texture* light_map = nullptr;
    public:
        void addOwnModel(Model* md);
        void addOwnLight(Light* lit);
  	private:
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
	public:
		Scene();
		Scene(Scene&& src) noexcept;
		Scene& operator=(Scene&& src) noexcept;
        ~Scene() noexcept;
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