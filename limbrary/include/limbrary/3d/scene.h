/*
    imdongye@naver.com
	fst: 2023-09-10
	lst: 2023-09-10

Note:
    < Scene : 렌더링단위 및 모델과 라이트 저장공간 >

    여러개의 scene을 동시에 보는것 고려됨.
    scene마다 light와 model들을 소유권이 있는것과 렌더링되는것을 나눈다.
    (model도 비슷하게 mesh, material, texture의 소유권과 렌더링을 나눠서 관리됨)
    즉 scene을 캐슁하기 위한 저장공간으로 쓸수도있음
    (예외로 model안의 material에 program은 소유권이 어플리케이션에 있음.


    Application 소유권 : Program, IBLight, Scene
    Scene의 소유권 : ModelData, Light
    Model의 소유권 : Texture, Material, Mesh

    Scene에서 alpha블랜딩이 없을때 가까운 물체를 배열의 앞에 배치해야 fs가 덜 호출되어 성능상 이득.


    렌더링할수있는 ModelView객체는
    1. ModelView객체 ModelData를 참조하고
    2. Scene에서는 ModelView를 참조하고 
    3. ModelView객체가 업케스팅으로 ModelData를 소유(자기자신) 일수있수있고
    4. Scene에서 그러한 Data를 담고있는 View를 소유할수있다.

Todo:

*/

#ifndef __scene_h_
#define __scene_h_

#include <vector>
#include "model.h"
#include "light.h"
#include "../framebuffer.h"
#include "model.h"

namespace lim
{
	class Scene final : public NoCopyAndMove
	{
  	public:
        // using in render
		std::vector<OwnPtr<ModelView>> own_mdvs;
        std::vector<OwnPtr<LightDirectional>> own_dir_lits;
        std::vector<OwnPtr<LightSpot>> own_spot_lits;
        std::vector<OwnPtr<LightOmni>> own_omni_lits;
        const IBLight* ib_light = nullptr; // ref
        bool is_draw_env_map = false;
        int idx_LitMod = -1;

        // only contained in scene
        std::vector<ModelData*> src_mds;
        std::vector<OwnPtr<Program>> own_progs;
        std::vector<OwnPtr<IBLight>> own_ib_lits;

    public:
        Scene() = default;
        ~Scene() = default;

        void reset();

        ModelView*          addOwn(ModelView* md);
        ModelData*          addOwn(ModelData* md);
        LightDirectional*   addOwn(LightDirectional* lit);
        LightSpot*          addOwn(LightSpot* lit);
        LightOmni*          addOwn(LightOmni* lit);
        Program*            addOwn(Program* prog);
        IBLight*            addOwn(IBLight* lit, bool setUse = true);


        // use prog in mat
        void render(const IFramebuffer& fb, const Camera& cam, const bool isDrawLight = false);
	};
}



#endif