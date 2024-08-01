/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using namespace pbd;
using std::vector;

namespace {
    constexpr glm::vec3 G = {0, -9.8, 0};
}


void SoftBody::update(float dt)
{
    // (5) update v with external force (ex. G)
    for( int i=0; i<nr_ptcls; i++ ) {
        v_s[i] += f_s[i]*w_s[i]*dt;
        np_s[i] = poss[i] + v_s[i]*dt;
    }

    float sqdt = dt*dt;
    float alpha;



    



    alpha = compliance.dih_bend/sqdt;
    for( auto& c : c_dih_bends ) {
        c.project( *this, alpha );
    }

    alpha = compliance.iso_bend/sqdt;
    for( auto& c : c_iso_bends ) {
        c.project( *this, alpha );
    }

    alpha = compliance.glo_volume/sqdt;
    for( auto& c : c_g_volumes ) {
        c.project( *this, alpha );
    }

    for( auto& c : c_fixes ) {
        c.project( *this );
    }





    alpha = compliance.stretch/sqdt;
    for( auto& c : c_dist_bends ) {
        c.project( *this, alpha );
    }
    alpha = compliance.stretch/sqdt;
    for( auto& c : c_shears ) {
        c.project( *this, alpha );
    }
    alpha = compliance.stretch/sqdt;
    for( auto& c : c_stretchs ) {
        c.project( *this, alpha );
    }







    // (12) update x, v
    for( int i=0; i<nr_ptcls; i++ ) {
        v_s[i] = (np_s[i] - poss[i])/dt;
        poss[i] = np_s[i];
    }
}


void Simulator::update(float dt) 
{
    dt = dt/nr_steps;

    for( int step=0; step<nr_steps; step++ )
    {
        for( auto body : bodies )
        {
            // add external force
            for( int i=0; i<body->nr_ptcls; i++ )
            {
                float pw = body->w_s[i];
                if( pw == 0.f ) {
                    body->f_s[i] = vec3(0);
                    continue;
                }
                body->f_s[i] = G/pw;
                body->f_s[i] -= (air_drag / pw / body->total_mass) * body->v_s[i];
            }
            
            // pbd update and solve
            body->update( dt );
        }
        for( auto body : bodies )
        {
            for( int i=0; i<body->nr_ptcls; i++ )
            {
                float w = body->w_s[i];
                if( w == 0.f )
                    continue;
                vec3& p = body->poss[i];
                vec3& v = body->v_s[i];
                vec3 sNor, vNor, vTan;
                for( auto pC : static_colliders ) {
                    float inter_dist = -pC->getSdNor( p, sNor )+ptcl_radius;
                    if( inter_dist < 0 )
                        continue;
                    p = p + sNor*inter_dist;
                    vNor = dot( sNor, v ) * sNor;
                    vTan = v - vNor;
                    v = (pC->friction * body->friction * vTan) - (pC->restitution * body->restitution * vNor);
                }
            }
        }
    }

    for( auto body : bodies ) {
        if( body->upload_to_buf ) {
            body->restorePosBuf();
        }
    }
}



Simulator::~Simulator()
{
    clearRefs();
}
void Simulator::clearRefs() {
    static_colliders.clear();
    bodies.clear();
}