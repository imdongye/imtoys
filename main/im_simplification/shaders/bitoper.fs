//
// todo: 노멀맵에서 범프맵을 만들때 적분을 못해서 미분값을 가지고 포아송 방정식(자코비, 가우스세이델)을 풀면된다.
#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

/* global const */
const float gamma = 2.2f;

/* camera */
uniform vec3 cameraPos;
uniform mat4 viewPat;
uniform mat4 projMat;

/* light */
uniform vec3 lightPos = vec3(1,1,0);
uniform vec3 lightColor = vec3(1);
uniform float lightInt = 0.8;
uniform int shadowEnabled = 0;
uniform mat4 shadowVP = mat4(1);
uniform sampler2D map_Shadow;

/* matarial */
uniform vec4 Kd;
uniform vec4 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform vec3 Tf;
uniform float Ni;
uniform int map_Flags;
uniform sampler2D map_Kd;
uniform sampler2D map_Ks;
uniform sampler2D map_Ka;
uniform sampler2D map_Ns;
uniform sampler2D map_Bump;
uniform float texDelta = 0.00001;
uniform float bumpHeight = 100;

mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
    fragColor = vec4(0);
    if( (map_Flags&(1<<4)) > 0 ) {
	    fragColor += vec4(1,0,0,1); // bump
    }
    if( (map_Flags&(1<<5)) > 0 ) {
	    fragColor += vec4(0,1,0,1); // nor
    }
	if( (map_Flags&(1<<0)) > 0 ) {
	    fragColor += vec4(0,0,1,1); // Kd
    }
}
