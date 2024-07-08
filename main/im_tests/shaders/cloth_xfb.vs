#version 460 core

layout(location=0) in vec4 aPosm;
layout(location=1) in vec4 aPrevPosm;

layout(xfb_buffer=0, xfb_offset=0) out vec4 out_posm;
layout(xfb_buffer=1, xfb_offset=0) out vec4 out_prev_posm;

uniform samplerBuffer tex_posm;
uniform samplerBuffer tex_prev_posm;

uniform vec2 inter_size;
uniform int p_size_x;
uniform int p_size_y;
uniform float dt, ka, kr;
uniform float stretchKs, shearKs, bendingKs;
uniform float stretchKd, shearKd, bendingKd;
uniform vec3 gravity;



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
float cur_m;
vec3 cur_prev_pos;
vec3 cur_vel;


bool isOutSide(ivec2 p) {
    return p.x<0 || p.x>p_size_x || p.y<0 || p.y>p_size_y;
}
vec3 getSpringForce(ivec2 dxy, float ks, float kd) {
    ivec2 p2_ixy = cur_ixy+dxy;
    if(isOutSide(p2_ixy)) {
        return vec3(0);
    }
    int p2_idx = p2_ixy.x + p2_ixy.y*p_size_x;
    vec3 p2_pos = texelFetch(tex_posm, p2_idx).xyz;
    vec3 p2_prev_pos = texelFetch(tex_prev_posm, p2_idx).xyz;
    vec3 p2_vel = (p2_pos-p2_prev_pos)/dt;

    vec3 diffP = p2_pos - cur_pos;
    vec3 diffV = p2_vel - cur_vel;

    float oriLength = length(inter_size*dxy);
    float curLength = length(diffP);
    vec3 dir = diffP/curLength;

    float force = ks * (curLength - oriLength);
    force += kd * dot(diffV, dir);

    return force*dir;
}


void main()
{
    if(dt == 0)
        return;
    const int index = gl_VertexID;
    cur_ixy = ivec2( index % p_size_x, index / p_size_x );
    cur_pos = aPosm.xyz;
    cur_m = aPosm.w; // todo early out
    cur_prev_pos = aPrevPosm.xyz;
    cur_vel = (cur_pos-cur_prev_pos)/dt; // todo dt 최적화

    vec3 F = vec3(0);

    F += gravity*cur_m;
    F -= ka*cur_vel;

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
    if(cur_m!=0)
        acc = F/cur_m;

    vec3 newVel = cur_vel + acc*dt;
    if(cur_pos.y<0 && newVel.y<0) {
        newVel.y = -kr*newVel.y;
        // todo kmu
    }
    vec3 newPos = cur_pos + newVel*dt;
    out_posm = vec4(newPos, cur_m);
    out_prev_posm = vec4(cur_pos, cur_m);

    // vec4 myPos = texelFetch(tex_posm, index);
    // out_posm = myPos + vec4(0.01,0,0, 0);
    // out_prev_posm = myPos + vec4(0.01,0,0, 0);
}


