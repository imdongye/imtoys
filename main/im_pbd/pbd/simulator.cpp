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
        if( w_s[i] == 0 )
            continue;
        f_s[i] = G/w_s[i];
        // ...
        v_s[i] += f_s[i]*w_s[i]*dt;
    }

    // (6) dampVel
    // ...

    // (7) update p
    for( int i=0; i<nr_ptcls; i++ ) {
        np_s[i] = poss[i] + v_s[i]*dt;
    }


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
}


void Simulator::updateCollision()
{
    for( const auto pBody : bodies ) {
        for( int i=0; i<pBody->nr_ptcls; i++ ) {
            vec3 p = pBody->np_s[i];
            ICollider* minColl;
            float minDist = glim::fmin;
            for( const auto pColl : static_colliders ) {
                float dist = pColl->getSD( p );
                if( dist < minDist ) {
                    minDist = dist;
                    minColl = pColl;
                }
            }
            if( minDist < ptcl_radius) {

            }
        }
    }
}

void SoftBody::updateX(float dt)
{
    for( auto& c : c_fixes ) {
        c.project( *this );
    }

    // (12) update x, v
    for( int i=0; i<nr_ptcls; i++ ) {
        if( w_s[i] == 0 )
            continue;
        if( cs_s[i].norh.w < 0 ) {

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
            body->updateP( dt );
        }

        updateCollision();

        for( auto body : bodies )
        {
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