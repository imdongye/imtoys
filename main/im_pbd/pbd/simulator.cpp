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


void SoftBody::updateP(float dt)
{
    // (5) update v with external force (ex. G)
    for( int i=0; i<nr_ptcls; i++ ) {
        v_s[i] += G*dt;
    }

    // (6) dampVel
    // ...

    // (7) update p
    for( int i=0; i<nr_ptcls; i++ ) {
        np_s[i] = poss[i] + v_s[i]*dt;
    }
}

void SoftBody::updateX(float dt)
{
    float sqdt = dt*dt;
    float alpha;


    alpha = compliance.stretch/(200.f*sqdt);
    for( auto& c : c_stretchs ) {
        c.project( *this, alpha );
    }

    alpha = compliance.shear/(200.f*sqdt);
    for( auto& c : c_shears ) {
        c.project( *this, alpha );
    }

    alpha = compliance.dist_bend/(200.f*sqdt);
    for( auto& c : c_dist_bends ) {
        c.project( *this, alpha );
    }


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

    // collision
    // for( auto& c : c_colls )

    for( auto& c : c_fixes ) {
        c.project( *this );
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
        for( auto body : bodies ) {
            body->updateP( dt );
        }

        // generate collision constraints
        for( auto body : bodies ) {
            for( int i=0; i<body->nr_ptcls; i++ ) {
                if( body->np_s[i].y<0 ) {
                    body->np_s[i].y = 0;
                }
            }
        }

        for( auto body : bodies ) {
            body->updateX( dt );
        }
    }


    for( auto body : bodies ) {
        if( body->update_buf ) {
            body->restorePosBuf();
        }
    }
}



Simulator::~Simulator()
{
    clear();
}
void Simulator::clear() {
    for( auto body : bodies ) {
        delete body;
    }
    bodies.clear();
}