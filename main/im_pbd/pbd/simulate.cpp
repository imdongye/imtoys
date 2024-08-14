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



void SoftBody::subStepConstraintProject(float dt)
{
    float sqdt = dt*dt;
    float alpha;

    // Todo: tet volume

    // c_volume.projectImpulse( *this, params.pressure, dt );
    // alpha = params.inv_stiff_volume/sqdt;
    // c_volume.projectXpbd( *this, params.pressure, alpha );

    
    alpha = (1.f+params.inv_stiff_dih_bend)/sqdt;
    for( auto& c : c_dih_bends ) {
        c.project( *this, alpha );
    }
    alpha = params.inv_stiff_iso_bend/sqdt;
    for( auto& c : c_iso_bends ) {
        c.project( *this, alpha );
    }
    
    // Todo: skinning

    float distAlpha = params.inv_stiff_dist/sqdt;
    alpha = distAlpha / params.bend_pct;
    for( auto& c : c_stretchs ) {
        c.project( *this, alpha );
    }
    alpha = distAlpha / params.shear_pct;
    for( auto& c : c_shears ) {
        c.project( *this, alpha );
    }
    alpha = distAlpha / params.stretch_pct;
    for( auto& c : c_dist_bends ) {
        c.project( *this, alpha );
    }


    alpha = params.inv_stiff_point/sqdt;
    for( auto& c : c_points ) {
        c.project( *this, alpha );
    }
}



void SoftBody::applyCollision(float dt, const vector<ICollider*>& colliders)
{
    for( int i=0; i<nr_ptcls; i++ )
    {
        if( w_s[i] == 0.f )
            continue;
        vec3 x = x_s[i];
        vec3 v = (x - prev_x_s[i]) / dt;

        vec3 surfNor, vNor, vTan;
        for( auto pC : colliders ) {
            float signedDist = pC->getSdNor( x, surfNor )-ptcl_radius;
            if( signedDist > 0 )
                continue;
            x -= signedDist*surfNor;
            vNor = dot( surfNor, v ) * surfNor;
            vTan = v - vNor;
            v = (pC->friction * friction * vTan) - (pC->restitution * restitution * vNor);
        }

        prev_x_s[i] = x;
        x_s[i] = x;
        v_s[i] = v;
    }
}

void SoftBody::applyCollisionInterSubstep(const vector<ICollider*>& colliders)
{
    for( int i=0; i<nr_ptcls; i++ )
    {
        if( w_s[i] == 0.f )
            continue;
        vec3 x = x_s[i];
        vec3 v = v_s[i];

        vec3 surfNor, vNor, vTan;
        for( auto pC : colliders ) {
            float signedDist = pC->getSdNor( x, surfNor )-ptcl_radius;
            if( signedDist > 0 )
                continue;
            x -= signedDist*surfNor;
            vNor = dot( surfNor, v ) * surfNor;
            vTan = v - vNor;
            v = (pC->friction * friction * vTan) - (pC->restitution * restitution * vNor);
        }

        x_s[i] = x;
        v_s[i] = v;
    }
}



void SoftBody::update(float dt, const PhyScene& scene)
{
    float subDt = dt/nr_steps;

    for( int step=0; step<nr_steps; step++ )
    {
        // add external force
        for( int i=0; i<nr_ptcls; i++ )
        {
            if( w_s[i] == 0.f ) {
                p_s[i] = x_s[i];
                continue;
            }
            vec3 acc = scene.G;
            acc -= (scene.air_drag * inv_body_mass) * v_s[i];
            v_s[i] += acc*subDt;
            p_s[i] = x_s[i] + v_s[i]*subDt;
        }
        
        // pbd update and solve(projection)
        subStepConstraintProject( subDt );

        for( int i=0; i<nr_ptcls; i++ ) {
            v_s[i] = (p_s[i] - x_s[i])/subDt;
            x_s[i] = p_s[i];
        }

        applyCollisionInterSubstep( scene.colliders );
    }

    c_volume.applyImpulse( *this, params.pressure, dt );

    // applyCollision(dt, scene.colliders);
}


void PhyScene::update(float dt) 
{
    for( auto body : bodies ) {
        body->update( dt, *this );
    }
}