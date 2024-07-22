/*
    2024-07-17 / imdongye

*/

#ifndef __app_pbd_h_
#define __app_pbd_h_

#include <glm/glm.hpp>
#include <limbrary/model_view/mesh.h>
#include <vector>


namespace pbd
{
    // adjust info in mesh VS triangle?
    struct Edge {
        glm::uvec2 idx_ps;
        glm::uvec2 idx_ts;
        // current first lambda
        float length, fst_length, lam_length;
        float cangle, fst_cangle, lam_cangle;
        Edge(const glm::uvec2& idxPs, const glm::uvec2& idxTs, float len, float cAng)
            : idx_ps(idxPs), idx_ts(idxTs), length(len), fst_length(len), cangle(cAng), fst_cangle(cAng)
        {}
    };
    struct Mesh {
        std::vector<glm::vec4> x_s; // pos
        std::vector<glm::vec4> p_s; // temp pos
        std::vector<glm::vec4> v_s;
        std::vector<glm::uvec3> tris;
        std::vector<Edge> edges;
        glm::uint nr_ptcls, nr_edges, nr_tris;
        float volume, fst_volume, lam_volume;

        Mesh(const lim::Mesh& src);
        float updateVolume();
    };

    void simulate( Mesh& mesh, float dt );
}

#endif