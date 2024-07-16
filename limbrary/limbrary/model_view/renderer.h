/*

2023-09-10 / imdongye

< Scene : 렌더링단위 및 모델과 라이트 저장공간 >

Note:
여러개의 scene을 동시에 보는것 고려됨.
scene마다 light와 model들을 소유권이 있는것과 렌더링되는것을 나눈다.
(model도 비슷하게 mesh, material, texture의 소유권과 렌더링을 나눠서 관리됨)
즉 scene을 캐슁하기 위한 저장공간으로 쓸수도있음
(예외로 model안의 material에 program은 소유권이 어플리케이션에 있음.


Application 소유권 : Program, IBLight, Scene
Scene의 소유권 : Model, Light
Model의 소유권 : Texture, Material, Mesh

Scene에서 alpha블랜딩이 없을때 가까운 물체를 배열의 앞에 배치해야 fs가 덜 호출되어 성능상 이득.


렌더링할수있는 ModelView객체는
1. ModelView객체 ModelData를 참조하고
2. Scene에서는 ModelView를 참조하고 
3. ModelView객체가 업케스팅으로 ModelData를 소유(자기자신) 일수있수있고
4. Scene에서 그러한 Data를 담고있는 View를 소유할수있다.

Todo:


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
    // pre-filtered, split-sum
    class IBLight
    {
    public:
        static constexpr int nr_roughness_depth = 10;
        Texture map_Light, map_Irradiance, map_PreFilteredBRDF;
        Texture3d map_PreFilteredEnv;
        bool is_baked = false;

    public:
        IBLight(const IBLight&)	           = delete;
		IBLight(IBLight&&)			       = delete;
		IBLight& operator=(const IBLight&) = delete;
		IBLight& operator=(IBLight&&)      = delete;

        IBLight(const char* path = nullptr);
        virtual ~IBLight() = default;

        bool setMapAndBake(std::string_view path);
        void deinitGL();
        GLuint getTexIdLight() const;
        GLuint getTexIdIrradiance() const;
        GLuint getTexIdPreFilteredEnv() const;
        GLuint getTexIdPreFilteredBRDF() const;

        void setUniformTo(const Program& prg) const;

    };


	class Scene
	{
  	public:
		std::vector<ModelView*> own_mds;
        std::vector<ILight*> own_lits;
		std::vector<const ModelView*> mds;
		std::vector<const ModelView*> mds_skinned;
		std::vector<const ILight*> lights;
        const IBLight* ib_light = nullptr;
        bool is_draw_env_map = false;

    public:
		Scene(const Scene&)	           = delete;
		Scene(Scene&&)			       = delete;
		Scene& operator=(const Scene&) = delete;
		Scene& operator=(Scene&&)      = delete;

        Scene() = default;
        virtual ~Scene();

        void releaseData();

        ModelView* addOwn(ModelView* md);
        ModelView* addOwnSkinned(ModelView* md);
        ILight* addOwn(ILight* lit);
        const ModelView* addRef(const ModelView* md);
        const ModelView* addRefSkinned(const ModelView* md);
        const ILight* addRef(const ILight* lit);

	};


    // use prog in mat
    void render( const IFramebuffer& fb,
                 const Camera& cam,
                 const Scene& scn,
                 const bool isDrawLight = false );
}



#endif