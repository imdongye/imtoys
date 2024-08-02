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
        p_s[i] = x_s[i] + v_s[i]*dt;
    }

    float sqdt = dt*dt;
    float alpha;



    
    
    if( c_g_volume.enabled ) {
        alpha = compliance.glo_volume/sqdt;
        c_g_volume.project( *this, alpha );
    }


    alpha = compliance.dih_bend/sqdt;
    for( auto& c : c_dih_bends ) {
        c.project( *this, alpha );
    }

    alpha = compliance.iso_bend/sqdt;
    for( auto& c : c_iso_bends ) {
        c.project( *this, alpha );
    }

    
    // alpha = compliance.point/sqdt;
    // for( auto& c : c_points ) {
    //     c.project( *this, alpha );
    // }



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
        v_s[i] = (p_s[i] - x_s[i])/dt;
        x_s[i] = p_s[i];
    }
}


void Simulator::update(float dt) 
{
    float subDt = dt/nr_steps;

    
    for( auto body : bodies ) for( int i=0; i<body->nr_ptcls; i++ )
    {
        body->x_s[i] = body->poss[i];
    }

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
            
            // pbd update and solve(projection)
            body->update( subDt );
        }
    }


    // collision
    for( auto body : bodies )
    {
        for( int i=0; i<body->nr_ptcls; i++ )
        {
            float w = body->w_s[i];
            if( w == 0.f )
                continue;
            vec3& x = body->poss[i];
            vec3& p = body->p_s[i];
            vec3& v = body->v_s[i];
            v = (p - x) / dt;

            vec3 sNor, vNor, vTan;
            for( auto pC : static_colliders ) {
                float inter_dist = -pC->getSdNor( p, sNor )+ptcl_radius;
                if( inter_dist < 0 )
                    continue;
                p += inter_dist*sNor;
                vNor = dot( sNor, v ) * sNor;
                vTan = v - vNor;
                v = (pC->friction * body->friction * vTan) - (pC->restitution * body->restitution * vNor);
            }
            x = p;
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