#include <limbrary/tools/render.h>
#include <limbrary/program.h>
#include <limbrary/tools/asset_lib.h>


void lim::drawEnvSphere(const Texture& map, const glm::mat4& mtx_View, const glm::mat4& mtx_Proj) {
    const Program& prog = asset_lib::prog_env->use();
    prog.setUniform("mtx_Model", glm::mat4(1));
    prog.setUniform("mtx_View", mtx_View);
    prog.setUniform("mtx_Proj", mtx_Proj);
    prog.setTexture("map_Light", map.getTexId());
    asset_lib::env_sphere->bindAndDrawGL();
}