#include <limbrary/3d/scene.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/render.h>
#include <limbrary/tools/text.h>

using namespace std;
using namespace lim;


void Scene::reset() {
    own_mds.clear();
    own_dir_lits.clear();
    own_spot_lits.clear();
    own_omni_lits.clear();
    ib_light = nullptr;
    is_draw_env_map = false;
    idx_LitMod = -1;
    
    src_mds.clear();
    own_progs.clear();
    own_ib_lits.clear();
}

ModelView* Scene::addOwn(ModelView* md)  {
    own_mds.push_back(md);
    return md;
}
ModelData* Scene::addOwn(ModelData* md)  {
    assert( md->src_md == md );
    own_mds.push_back((ModelView*)md);
    src_mds.push_back(md);
    return md;
}
LightDirectional* Scene::addOwn(LightDirectional* lit) {
    own_dir_lits.push_back(lit);
    return lit;
}
LightSpot* Scene::addOwn(LightSpot* lit) {
    own_spot_lits.push_back(lit);
    return lit;
}
LightOmni* Scene::addOwn(LightOmni* lit) {
    own_omni_lits.push_back(lit);
    return lit;
}
Program* Scene::addOwn(Program* prog) {
    own_progs.push_back(prog);
    return prog;
}
IBLight* Scene::addOwn(IBLight* lit, bool setUse) {
    own_ib_lits.push_back(lit);
    if( setUse ) {
        ib_light = lit;
        is_draw_env_map = true;
    }
    return lit;
}


/*

Note:
같은 Mesh를 여러번 draw할때 중복 bind를 줄이기위해 curMesh로 확인하고있다.
하지만 일반적인 모델의 경우 Mesh는 모두 다르기 때문에 성능을 위해서는
중복 Mesh를 위한 render함수를 따로 분리하는게 좋다.

    요구사항
material이 바뀌면 1.mat바인딩
program이 바뀌면 1.use 2.mat바인딩(setProg)
mesh바뀌면 1.ms바인딩
마지막 drawcall

*/
void Scene::render( const IFramebuffer& fb, const Camera& cam, const bool isDrawLight )
{
    const Program& shadowStatic = *asset_lib::prog_shadow_static;
    const Program& shadowSkinned = *asset_lib::prog_shadow_skinned;

    // todo: update menually
    for( auto& md : own_mds ) {
        md->root.updateGlobalTransform(getMtxTf(md->tf_prev));
    }

    // bake shadow map
    for( auto& lit : own_dir_lits ) {
        lit->bakeShadowMap([&](const glm::mat4& mtx_View, const glm::mat4& mtx_Proj) {
            for( auto& md : own_mds ) {
                if( md->own_animator && md->own_animator->is_enabled ) {
                    shadowSkinned.use();
                    md->own_animator->setUniformTo(shadowSkinned);
                    shadowSkinned.setUniform("mtx_View", mtx_View);
                    shadowSkinned.setUniform("mtx_Proj", mtx_Proj);
                }
                else {
                    shadowStatic.use();
                    shadowStatic.setUniform("mtx_View", mtx_View);
                    shadowStatic.setUniform("mtx_Proj", mtx_Proj);
                }

                md->root.dfsRender([](const Mesh* ms, const Material* mat, const glm::mat4& mtxGlobal) {
                    Program::g_cur_ptr->setUniform("mtx_Model", mtxGlobal);
                    ms->bindAndDrawGL();
                    return true;
                });
            }
        });
    }

    // main rendering
    fb.bind();
    if( is_draw_env_map&& ib_light != nullptr ) {
        drawEnvSphere(ib_light->map_Light, cam.mtx_View, cam.mtx_Proj);
    }
    
    const Program* curProg = nullptr;
    const Material* curMat = nullptr;
    const Mesh* curMesh = nullptr;
    bool isProgChanged = true;
    bool isMatChanged = true;
    bool isMeshChanged = true;
    bool isModelChanged = true;

    for( const auto& md : own_mds ) {
        isModelChanged = true;
        md->root.dfsRender([&](const Mesh* ms, const Material* mat, const glm::mat4& mtxGlobal) {
            if( curMat != mat ) {
                curMat = mat;
                isMatChanged = true;
                if( curProg != mat->prog ) {
                    curProg = mat->prog;
                    isProgChanged = true;
                }
            }

            if( curMesh != ms ) {
                curMesh = ms;
                isMeshChanged = true;
            }
            if( isProgChanged ) {
                curProg->use();
                curMat->setUniformTo(*curProg);
                cam.setUniformTo(*curProg);
                for( const auto& lit : own_dir_lits ) {
                    lit->setUniformTo(*curProg);
                }
                if( ib_light ) {
                    ib_light->setUniformTo(*curProg);
                }
                if( idx_LitMod >=0 ) {
                    curProg->setUniform("idx_LitMod", idx_LitMod);
                }
                if( md->own_animator && md->own_animator->is_enabled ) {
                    md->own_animator->setUniformTo(*curProg);
                }
            }
            else {
                if( isMatChanged ) {
                    curMat->setUniformTo(*curProg);
                }
                if( isModelChanged ) {
                    if( md->own_animator && md->own_animator->is_enabled ) {
                        md->own_animator->setUniformTo(*curProg);
                    }
                }
            }
            if( isMeshChanged ) {
                curMesh->bindGL();
            }
            curProg->setUniform("mtx_Model", mtxGlobal);
            curMesh->drawGL();
            return true;
        });
    }

    if( isDrawLight ) {
        const Program& prog = asset_lib::prog_ndv->use();
        cam.setUniformTo(prog);

        // todo: diff color
        for( const auto& lit : own_dir_lits ) {
            prog.setUniform("mtx_Model", lit->tf.mtx);
            // todo: draw dir with line
            asset_lib::small_sphere->bindAndDrawGL();
        }
    }

    fb.unbind();
}