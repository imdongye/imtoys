/*

    2023.12.05 / im dongye

    SDFF signed distance fields formats

    version : 0.1

*/

#include <glm/glm.hpp>
#include "sdf_global.h"
#include "sdf_bridge.h"
#include <fstream>
#include <nlohmann/json.h>
using Json = nlohmann::json;


static const char* sdff_version = "0.1";


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
static void toJson(const ObjNode& obj, Json& json) {
    json["prim_type"]    = obj.prim_type;
    json["op_group"]     = obj.op_group;
    json["op_spec"]      = obj.op_spec;
    json["blendness"]    = obj.blendness;
    json["name"]         = obj.name;
    toJson(obj.position,      json["position"]);
    toJson(obj.scale,         json["scale"]);
    toJson(obj.euler_angles,  json["euler_angles"]);
    toJson(obj.mirror,        json["mirror"]);

    if( obj.prim_type!=PT_GROUP ) {
        json["roundness"] = obj.roundness;
        json["mat_idx"] = obj.mat_idx;
        return;
    }
    for(int i=0; i<obj.children.size(); i++) {
        toJson(*obj.children[i], json["children"][i]);
    }
}
void fromJson(ObjNode& obj, const Json& json) {
    obj.op_group =  json["op_group"];
    obj.op_spec =   json["op_spec"];
    obj.blendness = json["blendness"];
    fromJson(obj.position,      json["position"]);
    fromJson(obj.scale,         json["scale"]);
    fromJson(obj.euler_angles,  json["euler_angles"]);
    fromJson(obj.mirror,        json["mirror"]);
    obj.composeTransform();
    
    if( obj.prim_type!=PT_GROUP ) {
        obj.mat_idx = json["mat_idx"];
        obj.roundness = json["roundness"];
        obj.updateGlsl();
        return;
    }

    Json jchildren = json["children"];
    for( int i=0; i<jchildren.size(); i++ ) {
        Json jchild = jchildren[i];
        std::string childName = jchild["name"];
        PrimitiveType childPrimType = jchild["prim_type"];
        obj.children.push_back( new ObjNode(childName, childPrimType, &obj) );
        fromJson(*obj.children.back(), jchild);
    }
}
void toJson(const SdfMaterial& mat, Json& json) {
    json["name"]       = mat.name;
    json["idx"]        = mat.idx;
    toJson(mat.base_color, json["base_color"]);
    json["metalness"]  = mat.metalness;
    json["roughness"]  = mat.roughness;
}
void fromJson(SdfMaterial& mat, const Json& json) {
    mat.name = json["name"];
    mat.idx = json["idx"];
    fromJson(mat.base_color, json["base_color"]);
    mat.metalness = json["metalness"];
    mat.roughness = json["roughness"];
    mat.updateGlsl();
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
static void toJson(const lim::Light& lit, Json& json) {
    toJson(lit.position,  json["position"]);
    toJson(lit.pivot,     json["pivot"]);
    toJson(lit.color,     json["color"]);
    json["intensity"]= lit.intensity;
}
static void fromJson(lim::Light& lit, const Json& json) {
    fromJson(lit.position,  json["position"]);
    fromJson(lit.pivot,     json["pivot"]);
    fromJson(lit.color,     json["color"]);
    lit.intensity =         json["intensity"];
}

void lim::sdf::exportJson(std::filesystem::path path) {
    std::ofstream ofile;
    Json ojson;

    ojson["sdff_version"] = sdff_version;
    ojson["model_name"] = model_name;

    for(int i=0; i<nr_mats; i++) {
        toJson(materials[i], ojson["materials"][i]);
    }
    toJson(root, ojson["root"]);
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
void lim::sdf::importJson(std::filesystem::path path) {
    std::ifstream ifile;
    Json ijson;

    lim::log::pure("sdff importer version %s\n", sdff_version);

    lim::sdf::deinit();
    try {
        ifile.open(path);
        ifile >> ijson;
    } catch( std::ifstream::failure& e ) {
        lim::log::err("fail read : %s, what? %s \n", path.c_str(), e.what());
    }

    std::string sdffVersionOfFile = ijson["sdff_version"];
    lim::log::pure("sdff file version %s\n", sdffVersionOfFile.c_str());

    model_name = ijson["model_name"];
    
    for(int i=0; i<nr_mats; i++) {
        fromJson(materials[i], ijson["materials"][i]);
    }
    fromJson(root, ijson["root"]);
    fromJson(*camera, ijson["camera"]);
    fromJson(*light, ijson["light"]);

    selected_obj = ( root.children.size()>0 )?root.children.back():&root;
}