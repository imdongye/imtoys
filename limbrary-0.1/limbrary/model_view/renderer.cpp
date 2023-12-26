/*

Todo:
지금 render node의 메쉬에 해당하는 material이 null일때 default를 사용함.

*/

#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <limbrary/log.h>
#include <stack>
#include <limbrary/utils.h>

using namespace std;
using namespace lim;


bool lim::IBLight::setMap(const char* path) 
{
    if(!strIsSame(getExtension(path),"hdr")) {
        lim::log::err("need hdr ext to load ibl\n");
        return false;
    }
    log::pure("loading ibl .. ");
    /* map_Light */
    if(!map_Light.initFromFile(path, false)) {
        return false;
    }
    is_map_baked = false;
    return true;
}
void lim::IBLight::bakeMap() {

    FramebufferNoDepth fb(3, 32); // 3 channel, 32 bit
    Program iblProg("ibl baker");


    /* map_Irradiance */
	fb.resize(256, 128);
	iblProg.attatch("canvas.vs").attatch("bake_irr.fs").link();

	fb.bind();
	iblProg.use();
	iblProg.setTexture("map_Light", map_Light.tex_id, 0);
	AssetLib::get().screen_quad.drawGL();
	fb.unbind();

    map_Irradiance = fb.color_tex; // !! 텍스쳐 복사되면서 mipmap도 생성됨.


    /* map_PreFilteredEnv */
    map_PreFilteredEnv.width = map_Light.width;
    map_PreFilteredEnv.height = map_Light.height;
    map_PreFilteredEnv.depth = roughness_depth;
    map_PreFilteredEnv.updateFormat(3, 32);
    map_PreFilteredEnv.initGL();

	fb.resize(map_Light.width, map_Light.height);
    iblProg.deinitGL();
    iblProg.attatch("canvas.vs").attatch("bake_pfenv.fs").link();

    for(int i=0; i<roughness_depth; i++) {
        float roughness = (i+0.5)/roughness_depth;// 0.05~0.95
        fb.bind();
        iblProg.use();
        iblProg.setTexture("map_Light", map_Light.tex_id, 0);
        iblProg.setUniform("roughness", roughness);
        AssetLib::get().screen_quad.drawGL();
        fb.unbind();
        float* buf = fb.makeFloatPixelsBuf();

        map_PreFilteredEnv.setDataWithDepth(i, buf); // !! 텍스쳐 복사되면서 mipmap도 생성됨.
    }
    is_map_baked = true;
}
GLuint lim::IBLight::getTexIdLight() const {
    return map_Light.tex_id;
}
GLuint lim::IBLight::getTexIdIrradiance() const {
    return map_Irradiance.tex_id;
}
GLuint lim::IBLight::getTexIdPreFilteredEnv() const {
    return map_PreFilteredEnv.tex_id;
}

lim::IBLight::IBLight(IBLight&& src) noexcept {
    *this = std::move(src);
}
IBLight& lim::IBLight::operator=(IBLight&& src) noexcept {
    if(this!=&src) {
        map_Light = std::move(src.map_Light);
        map_Irradiance = std::move(src.map_Irradiance);
        map_PreFilteredEnv = std::move(src.map_PreFilteredEnv);
        is_map_baked = src.is_map_baked;
    }
    return *this;
}




void lim::Scene::addModel(Model* md)  {
    models.push_back(md);
}
void lim::Scene::addLight(Light* lit) {
    lights.push_back(lit);
}
void lim::Scene::addOwnModel(Model* md)  {
    models.push_back(md);
    my_mds.push_back(md);
}
void lim::Scene::addOwnLight(Light* lit) {
    lights.push_back(lit);
    my_lits.push_back(lit);
}
lim::Scene::Scene()
{    
}
lim::Scene::Scene(Scene&& src) noexcept
{
    *this = std::move(src);
}
Scene& lim::Scene::operator=(Scene&& src) noexcept {
    if(this!=&src) {
        releaseData();
        my_mds = std::move(src.my_mds);
        models = std::move(src.models);
        my_lits= std::move(src.my_lits);
        lights = std::move(src.lights);
        ib_light = std::move(src.ib_light);
        is_draw_env_map = src.is_draw_env_map;
    }
    return *this;
}
lim::Scene::~Scene() noexcept {
    releaseData();
}
void lim::Scene::releaseData() {
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


namespace
{
    inline int bindLightToProg(const Program& prog, const Light& lit, int activeSlot)
    {
        prog.setUniform("light_Pos", lit.position);
        prog.setUniform("light_Color", lit.color);
        prog.setUniform("light_Int", lit.intensity);

        prog.setUniform("shadow_Enabled", (lit.shadow_enabled)?1:0);
        if( lit.shadow_enabled ) {
            prog.setUniform("shadow_VP", lit.shadow_vp_mat);
            prog.setTexture("map_Shadow", lit.map_Shadow.getRenderedTex(), activeSlot++);
        }
        return activeSlot; // return next texture slot
    }
    inline int bindMatToProg(const Program& prog, const Material& mat, int activeSlot)
    {
        prog.setUniform("mat_BaseColor", mat.baseColor);
        prog.setUniform("mat_SpecColor", mat.specColor);
        prog.setUniform("mat_AmbientColor", mat.ambientColor);
        prog.setUniform("mat_EmissionColor", mat.emissionColor);
        prog.setUniform("mat_F0", mat.F0);

        prog.setUniform("mat_Transmission", mat.transmission);
        prog.setUniform("mat_Refraciti", mat.refraciti);
        prog.setUniform("mat_Opacity", mat.opacity);
        prog.setUniform("mat_Shininess", mat.shininess);
        prog.setUniform("mat_Roughness", mat.roughness);
        prog.setUniform("mat_Metalness", mat.metalness);
        prog.setUniform("mat_TexDelta", mat.bumpHeight);
        prog.setUniform("mat_BumpHeight", mat.texDelta);


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

            depthProg.setUniform("view_Mat", lit.shadow_view_mat);
            depthProg.setUniform("proj_Mat", lit.shadow_proj_mat);
            
            for( const Model* pMd : mds ) {
                depthProg.setUniform("model_Mat", pMd->model_mat);
                dfsNodeTree(&pMd->root, [](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
                    ms->drawGL();
                });
            }

            lit.map_Shadow.unbind();
        }
    }
}


void lim::render( const IFramebuffer& fb,
                const Program& prog,
                const Model& md )
{
    fb.bind();
    prog.use();
    prog.setUniform("model_Mat", md.model_mat);

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

void lim::render( const IFramebuffer& fb, 
                const Program& prog,
                const Camera& cam,
                const Model& md, 
                const Light& lit )
{
    bakeShadowMap( {&lit}, {&md} );

    fb.bind();
    
    prog.use();
    prog.setUniform("camera_Pos", cam.position);
    prog.setUniform("proj_Mat", cam.proj_mat);
    prog.setUniform("view_Mat", cam.view_mat);
    prog.setUniform("model_Mat", md.model_mat);

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


void lim::render( const IFramebuffer& fb,
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
    // 
    std::function<void(const Program&)> curSetProg;
    int activeSlot = 0;

    if(scn.is_draw_env_map) {
        utils::drawEnvSphere(scn.ib_light->map_Light, cam.view_mat, cam.proj_mat);
    }

    for( const Model* pMd : scn.models ) {
        const Model& md = *pMd;
        nextMat = md.default_material;
        nextProg = md.default_material->prog;
        curSetProg = md.default_material->set_prog;

        dfsNodeTree(&md.root, [&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
            nextMat = (mat!=nullptr)?mat:md.default_material;
            if( nextMat->prog != nullptr )
                nextProg = nextMat->prog;
            if( nextMat->set_prog )
                curSetProg = nextMat->set_prog;

            if( curProg != nextProg ) {
                const Program& prog = *nextProg;
                activeSlot = 0;

                prog.use();
                prog.setUniform("camera_Pos", cam.position);
                prog.setUniform("view_Mat", cam.view_mat);
                prog.setUniform("proj_Mat", cam.proj_mat);

                for( const Light* pLit : scn.lights ) {
                    activeSlot = bindLightToProg(prog, *pLit, activeSlot);
                    break; //  Todo: 지금은 라이트 하나만
                }

                if( scn.ib_light ) {
                    prog.setTexture("map_Light", scn.ib_light->getTexIdLight(), activeSlot++);
                    prog.setTexture("map_Irradiance", scn.ib_light->getTexIdIrradiance(), activeSlot++);
                    // Todo: 3d를 2d로 업케스팅 강제로 막는 방법 없나
                    prog.setTexture3d("map_PreFilteredEnv", scn.ib_light->getTexIdPreFilteredEnv(), activeSlot++);
                }
            }

            if( curProg != nextProg || curMat != nextMat ) {
                curSetProg(*nextProg); // param active slot, return activeslot
                bindMatToProg(*nextProg, *nextMat, activeSlot);
                curMat = nextMat;
            }

            if( curProg != nextProg ) {
                curProg = nextProg;
            }

            curProg->setUniform("model_Mat", md.model_mat); // Todo: hirachi trnasformation
            ms->drawGL();
        });
    }

    fb.unbind();
}