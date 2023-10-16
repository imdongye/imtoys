#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace std;
using namespace lim;

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
        mat.set_program(prog);
        
        if( mat.map_Kd ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Kd->tex_id);
            prog.setUniform("map_Kd", activeSlot++);
        }
        if( mat.map_Ks ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ks->tex_id);
            prog.setUniform("map_Ks", activeSlot++);
        }
        if( mat.map_Ka ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ka->tex_id);
            prog.setUniform("map_Ka", activeSlot++);
        }
        if( mat.map_Ns ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ns->tex_id);
            prog.setUniform("map_Ns", activeSlot++);
        }
        if( mat.map_Bump ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            prog.setUniform("map_Bump", activeSlot++);

            if( mat.map_Flags & Material::MF_Height ) {
                prog.setUniform("texDelta", mat.texDelta);
                prog.setUniform("bumpHeight", mat.bumpHeight);
            }
        }

        prog.setUniform("Kd", mat.Kd);
        prog.setUniform("Ks", mat.Ks);
        prog.setUniform("Ka", mat.Ka);
        prog.setUniform("Ke", mat.Ke);
        prog.setUniform("Tf", mat.Tf);
        prog.setUniform("Ni", mat.Ni);
        prog.setUniform("map_Flags", mat.map_Flags);
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
        int activeSlot = 0;

        for( const Model* pMd : scn.models ) {
            const Model& md = *pMd;
            nextMat = md.default_material;
            nextProg = md.default_material->prog;

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

                    if(  curProg != nextProg || curMat != nextMat )
                    {
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



namespace lim
{
    void Scene::deleteOwn() {
        for( const Model* md: my_mds ){
            delete md;
        }
        my_mds.clear();
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
        deleteOwn();
        models = std::move(src.models);
        my_mds = std::move(src.my_mds);
        lights = std::move(src.lights);
        return *this;
    }
    Scene::~Scene() noexcept {
        deleteOwn();
    }
    void Scene::addModel( const Model* md, bool deleteWhenScnDeleted ) {
        models.push_back(md);
        if(deleteWhenScnDeleted)
            my_mds.push_back(md);
    }
    void Scene::subModel( const Model* md ) {
        auto it = std::find(models.begin(), models.end(), md);
        if (it != models.end())
            models.erase(it);
        it = std::find(my_mds.begin(), my_mds.end(), md);
        if (it != my_mds.end())
            my_mds.erase(it);
    }
    void Scene::addLight( const Light* lit ) {
        lights.push_back(lit);
    }
    void Scene::subLight( const Light* lit ) {
        auto it = std::find(lights.begin(), lights.end(), lit);
        if (it != lights.end())
            lights.erase(it);
    }
}