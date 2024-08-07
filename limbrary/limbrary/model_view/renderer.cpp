#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/log.h>
#include <stack>
#include <limbrary/g_tools.h>

using namespace std;
using namespace lim;


IBLight::IBLight(const char* path) {
    if( path ) {
        setMapAndBake(path);
    }
}

bool IBLight::setMapAndBake(std::string_view path) 
{
    if(!strIsSame(getExtension(path.data()),"hdr")) {
        log::err("need hdr ext to load ibl\n");
        return false;
    }
    log::pure("loading ibl .. ");
    /* map_Light */
    map_Light.s_wrap_param = GL_REPEAT;
    map_Light.t_wrap_param = GL_MIRRORED_REPEAT;
    map_Light.mag_filter = GL_LINEAR;
    map_Light.min_filter = GL_LINEAR_MIPMAP_NEAREST;// Todo: mipmap 쓰면 경계 검은색 나옴;;
    if(!map_Light.initFromFile(path, false)) {
        return false;
    }



    //
    //  Bake Prefiltered Light Maps
    //
    FramebufferNoDepth fb(3, 32); // 3 channel, 32 bit
    Program iblProg("ibl baker");


    /* map_Irradiance */
    fb.color_tex.s_wrap_param = GL_REPEAT;
    fb.color_tex.t_wrap_param = GL_MIRRORED_REPEAT;
    fb.color_tex.mag_filter = GL_LINEAR;
    fb.color_tex.min_filter = GL_LINEAR;// Todo: mipmap 쓰면 경계 검은색 나옴;;
	fb.resize(256, 128);
	iblProg.attatch("canvas.vs").attatch("bake_irr.fs").link();

	fb.bind();
	iblProg.use();
	iblProg.setTexture("map_Light", map_Light.tex_id);
	AssetLib::get().screen_quad.bindAndDrawGL();
	fb.unbind();

    map_Irradiance = fb.color_tex; // !! 텍스쳐 복사되면서 mipmap도 생성됨.


    /* map_PreFilteredEnv */
    // win에서 *0.25, mac에서 /10은 해야 동작함.
    const glm::vec2 pfenv_size = { map_Light.width/10.f,  map_Light.height/10.f }; 
    map_PreFilteredEnv.width = pfenv_size.x;
    map_PreFilteredEnv.height = pfenv_size.y;
    map_PreFilteredEnv.nr_depth = nr_roughness_depth;
    map_PreFilteredEnv.updateFormat(3, 32);
    map_PreFilteredEnv.s_wrap_param = GL_REPEAT;
    map_PreFilteredEnv.t_wrap_param = GL_MIRRORED_REPEAT;
    map_PreFilteredEnv.r_wrap_param = GL_CLAMP_TO_EDGE;
    map_PreFilteredEnv.mag_filter = GL_LINEAR;
    map_PreFilteredEnv.min_filter = GL_LINEAR;
    map_PreFilteredEnv.initGL();

	fb.resize(pfenv_size.x, pfenv_size.y);
    iblProg.deinitGL();
    iblProg.attatch("canvas.vs").attatch("bake_pfenv.fs").link();

    for(int i=0; i<nr_roughness_depth; i++) {
        float roughness = (i+0.5)/nr_roughness_depth;// 0.05~0.95
        fb.bind();
        iblProg.use();
        iblProg.setTexture("map_Light", map_Light.tex_id);
        iblProg.setUniform("roughness", roughness);
        AssetLib::get().screen_quad.bindAndDrawGL();
        fb.unbind();

        float* buf = fb.makeFloatPixelsBuf();
        map_PreFilteredEnv.setDataWithDepth(i, buf); // !! 텍스쳐 복사되면서 mipmap도 생성됨.
        delete buf;
    }

    /* map_PreFilteredBRDF */
    fb.color_tex.s_wrap_param = GL_CLAMP_TO_EDGE;
    fb.color_tex.t_wrap_param = GL_CLAMP_TO_EDGE;
    fb.color_tex.mag_filter = GL_LINEAR;
    fb.color_tex.min_filter = GL_LINEAR;
	fb.resize(128, 128);
    iblProg.deinitGL();
	iblProg.attatch("canvas.vs").attatch("bake_brdf.fs").link();

	fb.bind();
	iblProg.use();
	AssetLib::get().screen_quad.bindAndDrawGL();
	fb.unbind();

    map_PreFilteredBRDF = fb.color_tex; // !! 텍스쳐 복사되면서 mipmap도 생성됨.
    is_baked = true;
    return true;
}

void IBLight::deinitGL() {
    map_Light.deinitGL();
    map_Irradiance.deinitGL();
    map_PreFilteredEnv.deinitGL();
    map_PreFilteredBRDF.deinitGL();
}
GLuint IBLight::getTexIdLight() const {
    return map_Light.tex_id;
}
GLuint IBLight::getTexIdIrradiance() const {
    return map_Irradiance.tex_id;
}
GLuint IBLight::getTexIdPreFilteredEnv() const {
    return map_PreFilteredEnv.tex_id;
}
GLuint IBLight::getTexIdPreFilteredBRDF() const {
    return map_PreFilteredBRDF.tex_id;
}
void IBLight::setUniformTo(const Program& prg) const {
    prg.setTexture("map_Light", getTexIdLight());
    prg.setTexture("map_Irradiance", getTexIdIrradiance());
    prg.setTexture("map_PreFilteredBRDF", getTexIdPreFilteredBRDF());
    prg.setTexture3d("map_PreFilteredEnv", getTexIdPreFilteredEnv());
}







Scene::~Scene() {
    releaseData();
}
void Scene::releaseData() {
    for( ModelView* md: own_mds ){
        delete md;
    }
    for( ILight* lit: own_lits ){
        delete lit;
    }
    own_mds.clear();
    own_lits.clear();
    mds.clear();
    lights.clear();
}

ModelView* Scene::addOwn(ModelView* md)  {
    mds.push_back(md);
    own_mds.push_back(md);
    return md;
}
ILight* Scene::addOwn(ILight* lit) {
    lights.push_back(lit);
    own_lits.push_back(lit);
    return lit;
}
const ModelView* Scene::addRef(const ModelView* md)  {
    mds.push_back(md);
    return md;
}
const ILight* Scene::addRef(const ILight* lit) {
    lights.push_back(lit);
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
void lim::render( const IFramebuffer& fb,
                const Camera& cam,
                const Scene& scn,
                const bool isDrawLight )
{
    const Program& shadowStatic = AssetLib::get().prog_shadow_static;
    const Program& shadowSkinned = AssetLib::get().prog_shadow_skinned;

    // bake shadow map
    for( const ILight* lit : scn.lights ) {
        lit->bakeShadowMap([&](const glm::mat4& mtx_View, const glm::mat4& mtx_Proj) {
            for( const ModelView* md : scn.mds ) {
                if( md->animator.is_enabled ) {
                    shadowSkinned.use();
                    md->animator.setUniformTo(shadowSkinned);
                    shadowSkinned.setUniform("mtx_View", mtx_View);
                    shadowSkinned.setUniform("mtx_Proj", mtx_Proj);
                }
                else {
                    shadowStatic.use();
                    shadowStatic.setUniform("mtx_View", mtx_View);
                    shadowStatic.setUniform("mtx_Proj", mtx_Proj);
                }

                md->root.treversal([](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
                    g_cur_prog->setUniform("mtx_Model", transform);
                    ms->bindAndDrawGL();
                }, getMtxTf(md->tf_prev) );
            }
        });
    }

    // main rendering
    fb.bind();
    if( scn.is_draw_env_map ) {
        utils::drawEnvSphere(scn.ib_light->map_Light, cam.mtx_View, cam.mtx_Proj);
    }
    
    const Program* curProg = nullptr;
    const Material* curMat = nullptr;
    const Mesh* curMesh = nullptr;
    bool isProgChanged = true;
    bool isMatChanged = true;
    bool isMeshChanged = true;
    bool isModelChanged = true;

    for( const ModelView* md : scn.mds ) {
        isModelChanged = true;
        md->root.treversalEnabled([&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
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
                for( const ILight* lit : scn.lights ) {
                    lit->setUniformTo(*curProg);
                }
                if( scn.ib_light ) {
                    scn.ib_light->setUniformTo(*curProg);
                }
                if( md->animator.is_enabled ) {
                    md->animator.setUniformTo(*curProg);
                }
            }
            else {
                if( isMatChanged ) {
                    curMat->setUniformTo(*curProg);
                }
                if( isModelChanged ) {
                    if( md->animator.is_enabled ) {
                        md->animator.setUniformTo(*curProg);
                    }
                }
            }
            if( isMeshChanged ) {
                curMesh->bindGL();
            }
            curProg->setUniform("mtx_Model", transform);
            curMesh->drawGL();
        }, getMtxTf(md->tf_prev));
    }

    if( isDrawLight ) {
        const Program& prog = AssetLib::get().prog_ndv;
        prog.use();
        cam.setUniformTo(prog);

        // todo: diff color
        for( auto lit : scn.lights ) {
            prog.setUniform("mtx_Model", lit->tf.mtx);
            // todo: draw dir with line
            AssetLib::get().small_sphere.bindAndDrawGL();
        }
    }

    fb.unbind();
}