#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace std;

namespace
{
}

namespace lim
{
    void render( const Framebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit )
    {
        fb.bind();
        
        prog.use();
        prog.setUniform("cameraPos", cam.position);
        prog.setUniform("projMat", cam.proj_mat);
        prog.setUniform("viewMat", cam.view_mat);
        prog.setUniform("modelMat", md.model_mat);

        prog.setUniform("lightDir", lit.direction);
        prog.setUniform("lightColor", lit.color);
        prog.setUniform("lightInt", lit.intensity);
        prog.setUniform("lightPos", lit.position);

        for( Mesh* pMesh : md.meshes) {
            pMesh->drawGL();
        }

        fb.unbind();
    }

    void render( const Framebuffer& fb,
                 const Camera& cam,
                 const Scene& scn )
    {
        stack<Model::Node*> nodeStack;
        const Program& depthProg = *AssetLib::get().depth_prog;

        depthProg.use();

        for( Light* pLit : scn.lights) {
            const Light& lit = *pLit;

            if( lit.shadow_enabled == false ) {
                continue;
            }
            lit.map_Shadow.bind();

            depthProg.setUniform("viewMat", lit.view_mat);
            depthProg.setUniform("projMat", lit.proj_mat);

            for( Model* pMd : scn.models ) {
                const Model& md = *pMd;
                depthProg.setUniform("modelMat", md.model_mat);

                for( Mesh* pMs : md.meshes ) {
                    pMs->drawGL();
                }
            }

            lit.map_Shadow.unbind();
        }


        fb.bind();

        Material* curMat = nullptr;
        Material* nextMat = nullptr;
        Program* curProg = nullptr;
        Program* nextProg = nullptr;

        for( Model* pMd : scn.models ) {
            const Model& md = *pMd;
            nextMat = md.default_mat;
            nextProg = md.default_mat->prog;

            nodeStack.push(&(pMd->root));
            while( nodeStack.size()>0 ) {
                Model::Node* pNd = nodeStack.top();
                nodeStack.pop();
                for( Model::Node& child : pNd->childs ) {
                    nodeStack.push(&child);
                }

                for( Mesh* pMs : pNd->meshes ) {
                    const Mesh& ms = *pMs;
                    int activeSlot = 0;
                    if( ms.material != nullptr ) {
                        nextMat = ms.material;
                        if( nextMat->prog != nullptr )
                            nextProg = nextMat->prog;
                    }

                    if( curProg != nextProg ) {
                        const Program& prog = *nextProg;
                        curProg = nextProg;
                        
                        prog.use();
                        prog.setUniform("cameraPos", cam.position);
                        prog.setUniform("projMat", cam.proj_mat);
                        prog.setUniform("viewMat", cam.view_mat);

                        /* Todo: 지금은 라이트 하나만 */
                        for( Light* pLit : scn.lights ) {
                            const Light& lit = *pLit;
                            prog.setUniform("lightDir", lit.direction);
                            prog.setUniform("lightColor", lit.color);
                            prog.setUniform("lightInt", lit.intensity);
                            prog.setUniform("lightPos", lit.position);
                            prog.setUniform("shadowEnabled", (lit.shadow_enabled)?1:0);
                            if( lit.shadow_enabled ) {
                                prog.setUniform("shadowVP", lit.vp_mat);
                                glActiveTexture(GL_TEXTURE0 + activeSlot);
                                glBindTexture(GL_TEXTURE_2D, lit.map_Shadow.color_tex);
                                prog.setUniform("map_Shadow", activeSlot++);
                            }
                            break;
                        }
                    }

                    if(  curMat != nextMat ) {
                        const Program& prog = *curProg;
                        const Material& mat = *nextMat;
                        curMat = nextMat;

                        prog.setUniform("Kd", mat.Kd);
                        prog.setUniform("Ks", mat.Ks);
                        prog.setUniform("Ka", mat.Ka);
                        prog.setUniform("Ke", mat.Ke);
                        prog.setUniform("Tf", mat.Tf);
                        prog.setUniform("Ni", mat.Ni);

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
                            if( !mat.bumpIsNormal ) {
                                prog.setUniform("bumpHeight", mat.bump_height);
                                prog.setUniform("texDelta", mat.tex_delta);
                            }
                        }
                    }

                    curProg->setUniform("modelMat", md.model_mat); // todo
                    ms.drawGL();
                }
            }
        }

        fb.unbind();
    }
}
