#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/log.h>
#include <stack>

using namespace std;
using namespace lim;


namespace lim
{
    void Scene::addModel(const Model* md)
    {
        models.push_back(md);
    }
    void Scene::addLight(const Light* lit)
    {
        lights.push_back(lit);
    }
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
        : my_mds(std::move(src.my_mds))
        , models(std::move(src.models))
        , lights(std::move(src.lights))
    {
    }
    Scene& Scene::operator=(Scene&& src) noexcept {
        if(this==&src)
            return *this;
        for( const Model* md: my_mds ){
            delete md;
        }
        my_mds.clear();
        for( const Light* lit: my_lits ){
            delete lit;
        }
        my_lits.clear();

        my_mds = std::move(src.my_mds);
        models = std::move(src.models);
        my_lits = std::move(src.my_lits);
        lights = std::move(src.lights);

        return *this;
    }
    Scene::~Scene() noexcept {
        for( const Model* md: my_mds ){
            delete md;
        }
        for( const Light* lit: my_lits ){
            delete lit;
        }
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
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, lit.map_Shadow.color_tex);
            prog.setUniform("map_Shadow", activeSlot++);
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
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_BaseColor->tex_id);
            prog.setUniform("map_BaseColor", activeSlot++);
        }
        if( mat.map_Specular ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Specular->tex_id);
            prog.setUniform("map_Specular", activeSlot++);
        }
        if( mat.map_Bump ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            prog.setUniform("map_Bump", activeSlot++);

            if( mat.map_Flags & Material::MF_HEIGHT ) {
                prog.setUniform("texDelta", mat.texDelta);
                prog.setUniform("bumpHeight", mat.bumpHeight);
            }
        }
        if( mat.map_AmbOcc ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_AmbOcc->tex_id);
            prog.setUniform("map_AmbOcc", activeSlot++);
        }
        if( mat.map_Roughness ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Roughness->tex_id);
            prog.setUniform("map_Roughness", activeSlot++);
        }
        if( mat.map_Metalness ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Metalness->tex_id);
            prog.setUniform("map_Metalness", activeSlot++);
        }
        if( mat.map_Emission ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Emission->tex_id);
            prog.setUniform("map_Emission", activeSlot++);
        }
        if( mat.map_Opacity ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Opacity->tex_id);
            prog.setUniform("map_Opacity", activeSlot++);
        }
        return activeSlot;
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
                const Model& md = *pMd;
                depthProg.setUniform("modelMat", md.model_mat);

                for( const Mesh* ms : md.my_meshes ) {
                    ms->drawGL();
                }
            }

            lit.map_Shadow.unbind();
        }
    }
}

namespace lim
{
    void render( const Framebuffer& fb,
                 const Program& prog,
                 const Model& md )
    {
        fb.bind();
        prog.use();

        const Material* curMat = md.default_material;

        stack<const Model::Node*> nodeStack;
        nodeStack.push( &(md.root) );
        while( nodeStack.size()>0 ) {
            const Model::Node& node = *nodeStack.top();
            nodeStack.pop();
            for( const Model::Node& child : node.childs ) {
                nodeStack.push(&child);
            }

            for( int i=0; i<node.getNrMesh(); i++ ) {
                auto [ms, mat] = node.getMesh(i);
                if( mat!=nullptr ) {
                    curMat = mat;
                }
                bindMatToProg(prog, *curMat, 0);
                prog.setUniform("modelMat", md.model_mat);
                ms->drawGL();
            }
        }
        fb.unbind();
    }
    
    void render( const Framebuffer& fb, 
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

        stack<const Model::Node*> nodeStack;
        nodeStack.push( &(md.root) );

        while( nodeStack.size()>0 ) {
            const Model::Node& node = *nodeStack.top();
            nodeStack.pop();
            for( const Model::Node& child : node.childs ) {
                nodeStack.push(&child);
            }

            for( int i=0; i<node.getNrMesh(); i++ ) {
                auto [ms, mat] = node.getMesh(i);
                if( mat!=nullptr ) {
                    curMat = mat;
                }
                bindMatToProg(prog, *curMat, activeSlot);
                prog.setUniform("modelMat", md.model_mat);
                ms->drawGL();
            }
        }

        fb.unbind();
    }


    void render( const Framebuffer& fb,
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

            stack<const Model::Node*> nodeStack;
            nodeStack.push( &(pMd->root) );

            while( nodeStack.size()>0 ) {
                const Model::Node& node = *nodeStack.top();
                nodeStack.pop();
                for( const Model::Node& child : node.childs ) {
                    nodeStack.push(&child);
                }

                for( int i=0; i<node.getNrMesh(); i++ ) {
                    auto [ms, mat] = node.getMesh(i);

                    if( mat != nullptr ) {
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
                }
            }
        }

        fb.unbind();
    }
}



