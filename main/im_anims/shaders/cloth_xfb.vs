#version 460 core
precision highp float;

layout(location=0) in vec4 aPosm;
layout(location=1) in vec4 aVel;

layout(xfb_buffer=0, xfb_offset=0) out vec4 out_posm;
layout(xfb_buffer=1, xfb_offset=0) out vec4 out_vel;



uniform samplerBuffer tex_posm;
uniform samplerBuffer tex_vel;

uniform float cloth_p_mass;
uniform vec2 inter_p_size;
uniform ivec2 nr_p;
uniform float dt, ka, kr;
uniform float stretchKs, shearKs, bendingKs;
uniform float stretchKd, shearKd, bendingKd;
uniform vec3 gravity;

uniform mat4 mtx_geo_model;
uniform samplerBuffer tex_geo_pos;
uniform isamplerBuffer tex_geo_tri;
uniform int nr_geo_tris;



const ivec2 stretchDxy[] = {
    ivec2(1,0), ivec2(0,-1), ivec2(-1,0), ivec2(0,1),
};
const ivec2 shearDxy[] = {
    ivec2(1,-1), ivec2(-1,-1), ivec2(-1,1), ivec2(1,1)
};
const ivec2 bendingDxy[] = {
    ivec2(2,0), ivec2(0,-2), ivec2(-2,0), ivec2(0,2),
};

ivec2 cur_ixy;
vec3 cur_pos;
float cur_mass;
vec3 cur_vel;


bool isOutSide(ivec2 p) {
    return p.x<0 || p.x>=nr_p.x || p.y<0 || p.y>=nr_p.y;
}
vec3 getSpringForce(ivec2 dxy, float ks, float kd) {
    ivec2 p2_ixy = cur_ixy+dxy;
    if(isOutSide(p2_ixy)) {
        return vec3(0);
    }
    int p2_idx = p2_ixy.x + p2_ixy.y*nr_p.x;
    vec3 p2_pos = texelFetch(tex_posm, p2_idx).xyz;
    vec3 p2_vel = texelFetch(tex_vel, p2_idx).xyz;

    vec3 diffP = p2_pos - cur_pos;
    vec3 diffV = p2_vel - cur_vel;

    float oriLength = length(inter_p_size*dxy);
    float curLength = length(diffP);
    vec3 dir = diffP/curLength;
    float diffL = curLength - oriLength;

    float force = ks * diffL/oriLength;
    force += kd * dot(diffV*dt, dir)*oriLength; // todo

    return force*dir;
}

bool intersect(vec3 p1, vec3 p2, vec3 t1, vec3 t2, vec3 t3, out vec3 point) {
    vec3 diff, u, v, n, w0, w;
    float r, a, b;

    diff = p2 - p1;
    u = t2-t1;
    v = t3-t1;
    n = cross(u, v);

    w0 = t1-p1;
    a = dot(w0, n);
    b = dot(diff, n);

    r = a/b;
    if( r<0.0 || r>1.0 )
        return false;
    
    point = p1 + r*diff;
    w = t1-point;

    float uu, uv, vv, wu, wv, D;
    uu = dot(u,u);
    uv = dot(u,v);
    vv = dot(v,v);
    wu = dot(w,u);
    wv = dot(w,v);
    D = uv*uv - uu*vv;

    float s, t;
    s = (uv*wv - vv*wu) / D;
    if( s<0.0 || s>1.0 ) 
        return false;
    t = (uv*wu - uu*wv) / D;
    if( t<0.0 || t>1.0 ) 
        return false;

    return true;
}

void main()
{
    if(dt == 0 || aPosm.w==0.0) {
        out_posm = aPosm;
        out_vel = aVel;
        return;
    }
    const int index = gl_VertexID;
    cur_ixy = ivec2( index % nr_p.x, index / nr_p.x );
    
    cur_mass = cloth_p_mass;
    cur_pos = aPosm.xyz;
    cur_vel = aVel.xyz;

    vec3 F = vec3(0);
    F -= ka*cur_vel;
    F += gravity*cur_mass;

    for( int i=0; i<4; i++ ) {
        F += getSpringForce(stretchDxy[i], stretchKs, stretchKd);
    }
    for( int i=0; i<4; i++ ) {
        F += getSpringForce(shearDxy[i], shearKs, shearKd);
    }
    for( int i=0; i<4; i++ ) {
        F += getSpringForce(bendingDxy[i], bendingKs, bendingKd);
    }
    vec3 acc = vec3(0);
    if(cur_mass!=0)
        acc = F/cur_mass;
    vec3 newVel = cur_vel + acc*dt;
    
    if(cur_pos.y<0 && newVel.y<0) {
        newVel.y = -kr*newVel.y;
        // todo kmu
    }
    vec3 newPos = cur_pos + newVel*dt;
    // vec3 newPos = 2.0*cur_pos-cur_prev_pos + acc*dt*dt;


    vec3 t1, t2, t3, intersectPos;
    for( int i=0; i<nr_geo_tris; i++ ) {
        ivec3 tIdx = texelFetch(tex_geo_tri, i).xyz;
        t1 = vec3(mtx_geo_model*vec4(texelFetch(tex_geo_pos, tIdx.x).xyz,1));
        t2 = vec3(mtx_geo_model*vec4(texelFetch(tex_geo_pos, tIdx.y).xyz,1));
        t3 = vec3(mtx_geo_model*vec4(texelFetch(tex_geo_pos, tIdx.z).xyz,1));
        if( intersect(cur_pos, newPos, t1, t2, t3, intersectPos) ) {
            vec3 n = normalize(cross(t2-t1, t3-t1));
            newPos = intersectPos + reflect(newPos-intersectPos, n);
            newVel = kr* reflect(newVel, n);
            break;
        }
    }


    out_posm = vec4(newPos, 1);
    out_vel = vec4(newVel, 1);
}


