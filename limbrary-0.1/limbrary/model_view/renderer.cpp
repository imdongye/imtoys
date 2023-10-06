#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace std;
using namespace lim;

namespace
{
    inline void bindLightToProg(const Program& prog, const Light& lit, int& activeSlot)
    {
        prog.bind("lightPos", lit.position);
        prog.bind("lightColor", lit.color);
        prog.bind("lightInt", lit.intensity);
        prog.bind("shadowEnabled", (lit.shadow_enabled)?1:0);
        if( lit.shadow_enabled ) {
            prog.bind("shadowVP", lit.shadow_vp_mat);
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, lit.map_Shadow.color_tex);
            prog.bind("map_Shadow", activeSlot++);
        }
    }
    inline void bindMatToProg(const Program& prog, const Material& mat, int& activeSlot)
    {
        if( mat.map_Kd ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Kd->tex_id);
            prog.bind("map_Kd", activeSlot++);
        }
        if( mat.map_Ks ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ks->tex_id);
            prog.bind("map_Ks", activeSlot++);
        }
        if( mat.map_Ka ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ka->tex_id);
            prog.bind("map_Ka", activeSlot++);
        }
        if( mat.map_Ns ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Ns->tex_id);
            prog.bind("map_Ns", activeSlot++);
        }
        if( mat.map_Bump ) {
            glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            prog.bind("map_Bump", activeSlot++);

            if( mat.map_Flags & Material::MF_Bump ) {
                prog.bind("texDelta", mat.texDelta);
                prog.bind("bumpHeight", mat.bumpHeight);
            }
        }

        prog.bind("Kd", mat.Kd);
        prog.bind("Ks", mat.Ks);
        prog.bind("Ka", mat.Ka);
        prog.bind("Ke", mat.Ke);
        prog.bind("Tf", mat.Tf);
        prog.bind("Ni", mat.Ni);
        prog.bind("map_Flags", mat.map_Flags);
    }

    inline void bakeShadowMap(const std::vector<const Light*> lits, const std::vector<const Model*>& mds)
    {
        const Program& depthProg = AssetLib::get().depth_prog;

        depthProg.use();
        for( const Light* pLit : lits ) {
            const Light& lit = *pLit;

            if( lit.shadow_enabled == false ) {
                continue;
            }

            lit.map_Shadow.bind();

            depthProg.bind("viewMat", lit.shadow_view_mat);
            depthProg.bind("projMat", lit.shadow_proj_mat);

            for( const Model* pMd : mds ) {
                const Model& md = *pMd;
                depthProg.bind("modelMat", md.model_mat);

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
        for( const Mesh* ms : md.my_meshes ) {
            int activeSlot = 0;
            bindMatToProg(prog, *ms->material, activeSlot);
            ms->drawGL();
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
        prog.bind("cameraPos", cam.position);
        prog.bind("projMat", cam.proj_mat);
        prog.bind("viewMat", cam.view_mat);
        prog.bind("modelMat", md.model_mat);

        int activeSlot = 0;
        bindLightToProg(prog, lit, activeSlot);

        stack<const Model::Node*> nodeStack;
        nodeStack.push( &(md.root) );

        const Material* curMat = md.default_material;

        while( nodeStack.size()>0 ) {
            const Model::Node* pNd = nodeStack.top();
            nodeStack.pop();
            for( const Model::Node& child : pNd->childs ) {
                nodeStack.push(&child);
            }

            for( const Mesh* pMs : pNd->meshes ) {
                const Mesh& ms = *pMs;
                if( ms.material!=nullptr ) {
                    curMat = ms.material;
                }

                bindMatToProg(prog, *curMat, activeSlot);
                prog.bind("modelMat", md.model_mat);
                ms.drawGL();
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
        Material* curMat = nullptr;
        Material* nextMat = nullptr;
        Program* curProg = nullptr;
        Program* nextProg = nullptr;
        int activeSlot = 0;

        for( const Model* pMd : scn.models ) {
            const Model& md = *pMd;
            nextMat = md.default_material;
            nextProg = md.default_material->prog;

            stack<const Model::Node*> nodeStack;
            nodeStack.push( &(pMd->root) );

            while( nodeStack.size()>0 ) {
                const Model::Node* pNd = nodeStack.top();
                nodeStack.pop();
                for( const Model::Node& child : pNd->childs ) {
                    nodeStack.push(&child);
                }

                for( const Mesh* pMs : pNd->meshes ) {
                    const Mesh& ms = *pMs;

                    if( ms.material != nullptr ) {
                        nextMat = ms.material;
                        if( nextMat->prog != nullptr )
                            nextProg = nextMat->prog;
                    }

                    if( curProg != nextProg ) {
                        const Program& prog = *nextProg;
                        activeSlot = 0;

                        prog.use();
                        prog.bind("cameraPos", cam.position);
                        prog.bind("viewMat", cam.view_mat);
                        prog.bind("projMat", cam.proj_mat);

                        for( const Light* pLit : scn.lights ) {
                           bindLightToProg(prog, *pLit, activeSlot);
                            break; //  Todo: 지금은 라이트 하나만
                        }
                    }

                    if(  curProg != nextProg || curMat != nextMat )
                    {
                        int matRelativeActiveSlot = activeSlot;

                        bindMatToProg(*nextProg, *nextMat, matRelativeActiveSlot);

                        curMat = nextMat;
                    }

                    if( curProg != nextProg ) {
                        curProg = nextProg;
                    }

                    curProg->bind("modelMat", md.model_mat); // Todo: hirachi trnasformation
                    ms.drawGL();
                }
            }
        }

        fb.unbind();
    }
}
