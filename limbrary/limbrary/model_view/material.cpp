#include <limbrary/model_view/material.h>
#include <limbrary/asset_lib.h>

lim::Material::Material() {
    prog = &AssetLib::get().prog_ndv;
}

void lim::Material::setUniformTo(const Program& prg) const
{
    prg.setUniform("mat.BaseColor", BaseColor);
    prg.setUniform("mat.SpecColor", SpecColor);
    prg.setUniform("mat.AmbientColor", AmbientColor);
    prg.setUniform("mat.EmissionColor", EmissionColor);
    prg.setUniform("mat.F0", F0);

    prg.setUniform("mat.Transmission", Transmission);
    prg.setUniform("mat.Refraciti", Refraciti);
    prg.setUniform("mat.Opacity", Opacity);
    prg.setUniform("mat.Shininess", Shininess);
    prg.setUniform("mat.Roughness", Roughness);
    prg.setUniform("mat.Metalness", Metalness);
    if( map_Bump && map_Flags & Material::MF_HEIGHT) {
        prg.setUniform("mat.BumpHeight", BumpHeight);
        prg.setUniform("mat.TexDelta", TexDelta);
    }


    prg.setUniform("map_Flags", map_Flags);
    
    if( map_ColorBase ) prg.setTexture("map_BaseColor", map_ColorBase->tex_id);
    if( map_Specular )  prg.setTexture("map_Specular", map_Specular->tex_id);
    if( map_Bump )      prg.setTexture("map_Bump", map_Bump->tex_id);
    if( map_AmbOcc )    prg.setTexture("map_AmbOcc", map_AmbOcc->tex_id);
    if( map_Roughness ) prg.setTexture("map_Roughness", map_Roughness->tex_id);
    if( map_Metalness ) prg.setTexture("map_Metalness", map_Metalness->tex_id);
    if( map_Emission )  prg.setTexture("map_Emission", map_Emission->tex_id);
    if( map_Opacity )   prg.setTexture("map_Opacity", map_Opacity->tex_id);
    
    if( set_prog ) set_prog(prg);
}