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

    alpha = compliance.dih_bend/sqdt;
    for( auto& c : c_dih_bends ) {
        c.project( *this, alpha );
    }
    alpha = compliance.iso_bend/sqdt;
    for( auto& c : c_iso_bends ) {
        c.project( *this, alpha );
    }
    
    // Todo: skinning

    alpha = compliance.dist_bend/sqdt;
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
}


// From: https://namu.wiki/w/%EC%95%95%EB%A0%A5
void SoftBody::applyPressureImpulse(float dt)
{
    std::fill(pressureDVs.begin(), pressureDVs.end(), vec3(0));
    
    // PBD
    // for( const uvec3& tri : ptcl_tris ) {
    //     const vec3& p1 = p_s[tri.x];
    //     const vec3& p2 = p_s[tri.y];
    //     const vec3& p3 = p_s[tri.z];
    //     pressureDVs[tri.x] += cross(p3, p2)/6.f;
    //     pressureDVs[tri.y] += cross(p1, p3)/6.f;
    //     pressureDVs[tri.z] += cross(p2, p1)/6.f;
    // }

    // float C = body.getVolume() - pressure*ori_volume;
    // if( abs(C) < glim::feps )
    //     return;
    // float denom = alpha;
    // for( int i=0; i<body.nr_ptcls; i++ ) {
    //     denom += body.w_s[i]*length2(dPi[i]);
    // }
    // if( denom < glim::feps )
    //     return;
    // float lambda = C / denom;

    // for( int i=0; i<body.nr_ptcls; i++ ) {
    //     dPi[i] = lambda * body.w_s[i] * dPi[i];
    //     body.p_s[i] += dPi[i];
    //     if( body.w_s[i] > 0.f ) {
    //         dPi[i] = normalize(dPi[i])*0.1f;
    //     }
    // }
}


void SoftBody::applyCollision(float dt, const vector<ICollider*>& colliders)
{
    for( int i=0; i<nr_ptcls; i++ )
    {
        if( w_s[i] == 0.f )
            continue;
        vec3 p = p_s[i];
        vec3 v = (p - prev_x_s[i]) / dt;

        vec3 sNor, vNor, vTan;
        for( auto pC : colliders ) {
            float inter_dist = -pC->getSdNor( p, sNor )+ptcl_radius;
            if( inter_dist < 0 )
                continue;
            p += inter_dist*sNor;
            vNor = dot( sNor, v ) * sNor;
            vTan = v - vNor;
            v = (pC->friction * friction * vTan) - (pC->restitution * restitution * vNor);
        }

        prev_x_s[i] = p;
        p_s[i] = p;
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
    }


    applyCollision(dt, scene.colliders);


    // if( pressure>0.f ) {
    //     applyPressureImpulse(dt);
    // }


    if( upload_to_buf ) {
        uploadToBuf();
    }
}


void PhyScene::update(float dt) 
{
    for( auto body : bodies ) {
        body->update( dt, *this );
    }
}