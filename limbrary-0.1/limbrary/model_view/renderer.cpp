#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace std;

namespace
{
    int bindMatToProg(const lim::Program& prog, const lim::Material& mat, int activeSlot)
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

            if( mat.map_Flags & lim::Material::MF_Bump ) {
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

        return activeSlot;
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
            bindMatToProg(prog, *ms->material, 0);
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
        fb.bind();
        
        prog.use();
        prog.bind("cameraPos", cam.position);
        prog.bind("projMat", cam.proj_mat);
        prog.bind("viewMat", cam.view_mat);
        prog.bind("modelMat", md.model_mat);

        prog.bind("lightPos", lit.position);
        prog.bind("lightColor", lit.color);
        prog.bind("lightInt", lit.intensity);
        prog.bind("shadowEnabled", (lit.shadow_enabled)?1:0);
        if( lit.shadow_enabled ) {
            prog.bind("shadowVP", lit.shadow_vp_mat);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, lit.map_Shadow.color_tex);
            prog.bind("map_Shadow", 0);
        }

        for( const Mesh* ms : md.my_meshes) {
            ms->drawGL();
        }

        fb.unbind();
    }

    // node transformation 적용안함.
    void render( const Framebuffer& fb,
                 const Camera& cam,
                 const Scene& scn )
    {
        stack<const Model::Node*> nodeStack;
        const Program& depthProg = AssetLib::get().depth_prog;

        /* bake shadow map */
        depthProg.use();
        for( const Light* pLit : scn.lights) {
            const Light& lit = *pLit;

            if( lit.shadow_enabled == false ) {
                continue;
            }

            lit.map_Shadow.bind();

            depthProg.bind("viewMat", lit.shadow_view_mat);
            depthProg.bind("projMat", lit.shadow_proj_mat);

            for( const Model* pMd : scn.models ) {
                const Model& md = *pMd;
                depthProg.bind("modelMat", md.model_mat);

                for( const Mesh* ms : md.my_meshes ) {
                    ms->drawGL();
                }
            }

            lit.map_Shadow.unbind();
        }

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

            nodeStack.push(&(pMd->root));

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
                            const Light& lit = *pLit;
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
                            break; //  Todo: 지금은 라이트 하나만
                        }
                    }

                    if(  curProg != nextProg || curMat != nextMat ) {
                        const Program& prog = *nextProg;
                        const Material& mat = *nextMat;
                        
                        int matRelativeActiveSlot = activeSlot;

                        if( mat.map_Kd ) {
                            glActiveTexture(GL_TEXTURE0 + matRelativeActiveSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Kd->tex_id);
                            prog.bind("map_Kd", matRelativeActiveSlot++);
                        }
                        if( mat.map_Ks ) {
                            glActiveTexture(GL_TEXTURE0 + matRelativeActiveSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ks->tex_id);
                            prog.bind("map_Ks", matRelativeActiveSlot++);
                        }
                        if( mat.map_Ka ) {
                            glActiveTexture(GL_TEXTURE0 + matRelativeActiveSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ka->tex_id);
                            prog.bind("map_Ka", matRelativeActiveSlot++);
                        }
                        if( mat.map_Ns ) {
                            glActiveTexture(GL_TEXTURE0 + matRelativeActiveSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ns->tex_id);
                            prog.bind("map_Ns", matRelativeActiveSlot++);
                        }
                        if( mat.map_Bump ) {
                            glActiveTexture(GL_TEXTURE0 + matRelativeActiveSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
                            prog.bind("map_Bump", matRelativeActiveSlot++);

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
