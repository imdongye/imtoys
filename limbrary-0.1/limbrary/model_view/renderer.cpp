/*

Todo:
지금 render node에 material없을때 dfs에서 이전에 사용된 mat쓰는데 원래 부모의 mat을 사용하게 해야됨.

*/

#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/log.h>
#include <stack>

using namespace std;
using namespace lim;


namespace lim
{
    void Scene::addOwnModel(Model* md)
    {
        models.push_back(md);
        my_mds.push_back(md);
    }
    void Scene::addOwnLight(Light* lit)
    {
        lights.push_back(lit);
        my_lits.push_back(lit);
    }
    Scene::Scene()
    {    
    }
    Scene::Scene(Scene&& src) noexcept
    {
        my_mds = std::move(src.my_mds);
        models = std::move(src.models);
        my_lits =std::move(src.my_lits);
        lights = std::move(src.lights);
    }
    Scene& Scene::operator=(Scene&& src) noexcept {
        if(this!=&src) {
            releaseData();
            my_mds = std::move(src.my_mds);
            models = std::move(src.models);
            my_lits =std::move(src.my_lits);
            lights = std::move(src.lights);
        }
        return *this;
    }
    Scene::~Scene() noexcept {
        releaseData();
    }
    void Scene::releaseData() {
        for( const Model* md: my_mds ){
            delete md;
        }
        for( const Light* lit: my_lits ){
            delete lit;
        }
        my_mds.clear();
        my_lits.clear();
        models.clear();
        lights.clear();
    }
}

namespace
{
    inline int bindLightToProg(const Program& prog, const Light& lit, int activeSlot)
    {
        prog.setUniform("lightPos", lit.position);
        prog.setUniform("lightColor", lit.color);
        prog.setUniform("lightInt", lit.intensity);
        prog.setUniform("shadowEnabled", (lit.shadow_enabled)?1:0);
        if( lit.shadow_enabled ) {
            prog.setUniform("shadowVP", lit.shadow_vp_mat);
            prog.setTexture("map_Shadow", lit.map_Shadow.getRenderedTex(), activeSlot++);
        }
        return activeSlot; // return next texture slot
    }
    inline int bindMatToProg(const Program& prog, const Material& mat, int activeSlot)
    {
        prog.setUniform("baseColor", mat.baseColor);
        prog.setUniform("specColor", mat.specColor);
        prog.setUniform("ambientColor", mat.ambientColor);
        prog.setUniform("emissionColor", mat.emissionColor);

        prog.setUniform("transmission", mat.transmission);
        prog.setUniform("refraciti", mat.refraciti);
        prog.setUniform("opacity", mat.opacity);
        prog.setUniform("shininess", mat.shininess);
        prog.setUniform("roughness", mat.roughness);
        prog.setUniform("metalness", mat.metalness);
        prog.setUniform("f0", mat.f0);
        prog.setUniform("bumpHeight", mat.metalness);
        prog.setUniform("texDelta", mat.texDelta);


        prog.setUniform("map_Flags", mat.map_Flags);
        
        if( mat.map_BaseColor ) {
            prog.setTexture("map_BaseColor", mat.map_BaseColor->tex_id, activeSlot++);
        }
        if( mat.map_Specular ) {
            prog.setTexture("map_Specular", mat.map_Specular->tex_id, activeSlot++);
        }
        if( mat.map_Bump ) {
            prog.setTexture("map_Bump", mat.map_Bump->tex_id, activeSlot++);
            if( mat.map_Flags & Material::MF_HEIGHT ) {
                prog.setUniform("texDelta", mat.texDelta);
                prog.setUniform("bumpHeight", mat.bumpHeight);
            }
        }
        if( mat.map_AmbOcc ) {
            prog.setTexture("map_AmbOcc", mat.map_AmbOcc->tex_id, activeSlot++);
        }
        if( mat.map_Roughness ) {
            prog.setTexture("map_Roughness", mat.map_Roughness->tex_id, activeSlot++);
        }
        if( mat.map_Metalness ) {
            prog.setTexture("map_Metalness", mat.map_Metalness->tex_id, activeSlot++);
        }
        if( mat.map_Emission ) {
            prog.setTexture("map_Emission", mat.map_Emission->tex_id, activeSlot++);
        }
        if( mat.map_Opacity ) {
            prog.setTexture("map_Opacity", mat.map_Opacity->tex_id, activeSlot++);
        }
        return activeSlot;
    }

    // 편하고 코드 보기 좋아졌다. 메쉬마다 함수포인터로 점프해서 성능이 많이 안좋아질줄알았는데 큰차이없다.
    inline void dfsNodeTree(const Model::Node* root, function<void(const Mesh*, const Material*, const glm::mat4& transform)> hook) {
        stack<const Model::Node*> nodeStack;
        glm::mat4 transform = glm::mat4(1);
        nodeStack.push( root );
        while( nodeStack.size()>0 ) {
            const Model::Node& node = *nodeStack.top();
            nodeStack.pop();
            transform = node.transform*transform;
            for( const Model::Node& child : node.childs ) {
                nodeStack.push(&child);
            }
            for( int i=0; i<node.getNrMesh(); i++ ) {
                auto [ms, mat] = node.getMeshWithMat(i);
                hook(ms, mat, transform);
            }
        }
    }

    inline void bakeShadowMap(const std::vector<const Light*>& lits, const std::vector<const Model*>& mds)
    {
        const Program& depthProg = AssetLib::get().depth_prog;

        depthProg.use();
        for( const Light* pLit : lits ) {
            const Light& lit = *pLit;

            if( lit.shadow_enabled == false ) {
                continue;
            }

            lit.map_Shadow.bind();

            depthProg.setUniform("viewMat", lit.shadow_view_mat);
            depthProg.setUniform("projMat", lit.shadow_proj_mat);
            
            for( const Model* pMd : mds ) {
                depthProg.setUniform("modelMat", pMd->model_mat);
                dfsNodeTree(&pMd->root, [](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
                    ms->drawGL();
                });
            }

            lit.map_Shadow.unbind();
        }
    }
}

namespace lim
{
    void render( const IFramebuffer& fb,
                 const Program& prog,
                 const Model& md )
    {
        fb.bind();
        prog.use();
        prog.setUniform("modelMat", md.model_mat);

        const Material* curMat = md.default_material;

        dfsNodeTree(&md.root, [&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
            if( mat!=nullptr ) {
                curMat = mat;
            }
            bindMatToProg(prog, *curMat, 0);
            ms->drawGL();
        });

        fb.unbind();
    }
    
    void render( const IFramebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit )
    {
        bakeShadowMap( {&lit}, {&md} );

        fb.bind();
        
        prog.use();
        prog.setUniform("cameraPos", cam.position);
        prog.setUniform("projMat", cam.proj_mat);
        prog.setUniform("viewMat", cam.view_mat);
        prog.setUniform("modelMat", md.model_mat);

        int activeSlot = bindLightToProg(prog, lit, 0);

        const Material* curMat = md.default_material;

        dfsNodeTree(&md.root, [&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
            if( mat!=nullptr ) {
                curMat = mat;
            }
            bindMatToProg(prog, *curMat, activeSlot);
            ms->drawGL();
        });

        fb.unbind();
    }


    void render( const IFramebuffer& fb,
                 const Camera& cam,
                 const Scene& scn )
    {
        bakeShadowMap(scn.lights, scn.models);

        /* draw models */
        fb.bind();
        const Material* curMat = nullptr;
        const Material* nextMat = nullptr;
        const Program* curProg = nullptr;
        const Program* nextProg = nullptr;
        std::function<void(const Program&)> curSetProg;
        int activeSlot = 0;

        for( const Model* pMd : scn.models ) {
            const Model& md = *pMd;
            nextMat = md.default_material;
            nextProg = md.default_material->prog;
            curSetProg = md.default_material->set_prog;

            dfsNodeTree(&md.root, [&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
                if( mat!=nullptr ) {
                     nextMat = mat;
                    if( nextMat->prog != nullptr )
                        nextProg = nextMat->prog;
                    if( nextMat->set_prog )
                        curSetProg = nextMat->set_prog;
                }
                if( curProg != nextProg ) {
                    const Program& prog = *nextProg;
                    activeSlot = 0;

                    prog.use();
                    prog.setUniform("cameraPos", cam.position);
                    prog.setUniform("viewMat", cam.view_mat);
                    prog.setUniform("projMat", cam.proj_mat);

                    for( const Light* pLit : scn.lights ) {
                        activeSlot = bindLightToProg(prog, *pLit, activeSlot);
                        break; //  Todo: 지금은 라이트 하나만
                    }
                    prog.setUniform("useIBL", ( scn.light_map )?1:-1);
                    if( scn.light_map ) {
                        prog.setTexture("map_Light", scn.light_map->tex_id, activeSlot++);
                    }
                }

                if(  curProg != nextProg || curMat != nextMat ) {
                    curSetProg(*nextProg);
                    bindMatToProg(*nextProg, *nextMat, activeSlot);
                    curMat = nextMat;
                }

                if( curProg != nextProg ) {
                    curProg = nextProg;
                }

                curProg->setUniform("modelMat", md.model_mat); // Todo: hirachi trnasformation
                ms->drawGL();
            });
        }

        fb.unbind();
    }
}