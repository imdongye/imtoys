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
        glm::uint idx_p[2];
        glm::uint idx_t[2];
        float length, angle;
    };
    struct Mesh {
        std::vector<glm::vec4> poss;
        std::vector<glm::uvec3> tris;
        std::vector<Edge> edges;
        glm::uint nr_poss, nr_edges, nr_tris;
        float volume;

        Mesh(const lim::Mesh& src);
    };
}

#endif