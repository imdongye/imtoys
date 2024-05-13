/*

    2023.12.05 / im dongye

    SDFF signed distance fields formats with JSON

    version : 0.1

*/

#include <glm/glm.hpp>
#include "sdf_global.h"
#include "sdf_bridge.h"
#include <fstream>
#include <nlohmann/json.h>
using Json = nlohmann::json;


static const char* sdff_version = "0.2";


static void toJson(const glm::vec3& v, Json& json) {
    json = {v.x, v.y, v.z};
}
static void fromJson(glm::vec3& v, const Json& json) {
    v = { json[0], json[1], json[2]} ;
}
static void toJson(const glm::bvec3& v, Json& json) {
    json = {v.x, v.y, v.z};
}
static void fromJson(glm::bvec3& v, const Json& json) {
    v = { json[0], json[1], json[2]} ;
}
static void toJson(const sdf::Node* nod, Json& json) {
    toJson(nod->position,      json["position"]);
    toJson(nod->scale,         json["scale"]);
    toJson(nod->euler_angles,  json["euler_angles"]);
    toJson(nod->mirror,        json["mirror"]);
    json["op_group"]     = nod->op_group;
    json["op_spec"]      = nod->op_spec;
    json["blendness"]    = nod->blendness;
    json["roundness"]    = nod->roundness;

    json["name"]         = nod->name;
    json["is_group"]     = nod->is_group;

    if( nod->is_group ) {
        sdf::Group* grp = (sdf::Group*)nod;

        for(int i=0; i<grp->children.size(); i++) {
            toJson(grp->children[i], json["children"][i]);
        }
    }
    else {
        sdf::Object* obj = (sdf::Object*)nod;
        json["prim_type"] = obj->prim_type;
        json["mat_idx"] = obj->p_mat->idx;
        json["prim_idx"] = obj->prim_idx;
    }
}
void fromJson(sdf::Node* nod, const Json& json) {
    fromJson(nod->position,      json["position"]);
    fromJson(nod->scale,         json["scale"]);
    fromJson(nod->euler_angles,  json["euler_angles"]);
    fromJson(nod->mirror,        json["mirror"]);
    nod->op_group =  json["op_group"];
    nod->op_spec =   json["op_spec"];
    nod->blendness = json["blendness"];
    nod->roundness = json["roundness"];

    nod->name = json["name"];
    nod->is_group = json["is_group"];

    nod->composeTransform();
    
    if( nod->is_group && json.contains("children") ) {
        Json jchildren = json["children"];
        sdf::Group* grp = (sdf::Group*)nod;

        for( int i=0; i<jchildren.size(); i++ ) {
            Json jchild = jchildren[i];
            if(jchild["is_group"])
                grp->addGroupToBack();
            else
                grp->addObjectToBack(jchild["prim_type"]);

            fromJson(grp->children.back(), jchild);
        }
    }
    else {
        sdf::Object* obj = (sdf::Object*)nod;
        obj->prim_type = json["prim_type"];
        obj->p_mat = materials[json["mat_idx"]];
        obj->prim_idx = json["prim_idx"];
        obj->updateShaderData();
        return;
    }

    
}
void toJson(const sdf::Material& mat, Json& json) {
    json["name"]       = mat.name;
    json["idx"]        = mat.idx;
    toJson(mat.base_color, json["base_color"]);
    json["metalness"]  = mat.metalness;
    json["roughness"]  = mat.roughness;
}
void fromJson(sdf::Material& mat, const Json& json) {
    mat.name = json["name"];
    mat.idx = json["idx"];
    fromJson(mat.base_color, json["base_color"]);
    mat.metalness = json["metalness"];
    mat.roughness = json["roughness"];
    mat.updateShaderData();
}
static void toJson(const lim::CameraController& cam, Json& json) {
    toJson(cam.position,  json["position"]);
    toJson(cam.pivot,     json["pivot"]);
    json["fovy"] = cam.fovy;
}
static void fromJson(lim::CameraController& cam, const Json& json) {
    fromJson(cam.position,  json["position"]);
    fromJson(cam.pivot,     json["pivot"]);
    cam.fovy =              json["fovy"];
    cam.updateViewMat();
    cam.updateProjMat();
}
static void toJson(const lim::LightDirectional& lit, Json& json) {
    toJson(lit.tf.pos,   json["position"]);
    toJson(lit.tf.pivot, json["pivot"]);
    toJson(lit.Color,    json["color"]);
    json["intensity"]= lit.Intensity;
}
static void fromJson(lim::LightDirectional& lit, const Json& json) {
    fromJson(lit.tf.pos,    json["position"]);
    fromJson(lit.tf.pivot,  json["pivot"]);
    fromJson(lit.Color,     json["color"]);
    lit.Intensity =         json["intensity"];
}

void sdf::exportJson(std::filesystem::path path) {
    std::ofstream ofile;
    Json ojson;

    ojson["sdff_version"] = sdff_version;
    ojson["model_name"] = model_name;

    for(int i=0; i<materials.size(); i++) {
        toJson(*materials[i], ojson["materials"][i]);
        materials[i]->updateShaderData();
    }
    toJson((sdf::Object*)root, ojson["root"]);
    toJson(*camera, ojson["camera"]);
    toJson(*light, ojson["light"]);

    try {
        ofile.open(path);
        ofile << std::setw(4) << ojson << std::endl;
        ofile.close();
    } catch( std::ofstream::failure& e ) {
        lim::log::err("fail write : %s, what? %s \n", path.c_str(), e.what());
    }
}
void sdf::importJson(std::filesystem::path path) {
    std::ifstream ifile;
    Json ijson;

    lim::log::pure("sdff importer version %s\n", sdff_version);

    sdf::deinit();

    try {
        ifile.open(path);
        ifile >> ijson;
    } catch( std::ifstream::failure& e ) {
        lim::log::err("fail read : %s, what? %s \n", path.c_str(), e.what());
    }

    std::string sdffVersionOfFile = ijson["sdff_version"];
    lim::log::pure("sdff file version %s\n", sdffVersionOfFile.c_str());

    model_name = ijson["model_name"];
    
    int nr_mats = ijson["materials"].size();
    for(int i=0; i<nr_mats; i++) {
        materials.push_back(new Material());
        fromJson(*materials[i], ijson["materials"][i]);
    }
    root = new Group("root", nullptr);
    fromJson((sdf::Object*)root, ijson["root"]);
    serializeModel();
    fromJson(*camera, ijson["camera"]);
    fromJson(*light, ijson["light"]);
}