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
    // pre-filtered, split-sum
    class IBLight
    {
    public:
        static constexpr int nr_roughness_depth = 10;
        Texture map_Light, map_Irradiance, map_PreFilteredBRDF;
        Texture3d map_PreFilteredEnv;
        bool is_map_baked = false;
    public:
        bool setMap(const char* path);
        void bakeMap();
        void deinitGL();
        GLuint getTexIdLight() const;
        GLuint getTexIdIrradiance() const;
        GLuint getTexIdPreFilteredEnv() const;
        GLuint getTexIdPreFilteredBRDF() const;
        IBLight() = default;
        ~IBLight() noexcept = default;
        IBLight(IBLight&& src) noexcept;
		IBLight& operator=(IBLight&& src) noexcept;
    private:
        IBLight(const IBLight&) = delete;
		IBLight& operator=(const IBLight&) = delete;
    };

	class Scene
	{
  	public:
		std::vector<Model*> my_mds;
        std::vector<Light*> my_lits;
		std::vector<const Model*> models;
		std::vector<const Light*> lights;
        const IBLight* ib_light = nullptr;
        bool is_draw_env_map = false;
    public:
        void addModel(Model* md);
        void addLight(Light* lit);
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
        void releaseData();
	};

    void render( const IFramebuffer& fb,
                 const Program& prog,
                 const Model& md );

    // use prog in param
    void render( const IFramebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit );

    // use prog in mat
    void render( const IFramebuffer& fb,
                 const Camera& cam,
                 const Scene& scn );
}



#endif