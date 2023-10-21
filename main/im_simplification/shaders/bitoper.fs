#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform vec3 Tf;

uniform float d;
uniform float Tr;
uniform float Ns;
uniform float Ni;
uniform float roughness;

uniform int map_Flags;

uniform sampler2D map_Kd;
uniform sampler2D map_Ks;
uniform sampler2D map_Ka;
uniform sampler2D map_Ns;
uniform sampler2D map_Bump;
uniform float texDelta;
uniform float bumpHeight;

const float PI = 3.1415926535;
const int MF_None   = 0;
const int MF_Kd     = 1<<0;
const int MF_Ks     = 1<<1;
const int MF_Ka     = 1<<2;
const int MF_Ns     = 1<<3;
const int MF_Height = 1<<4;
const int MF_Nor    = 1<<5;


mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
    FragColor = vec4(0,0,0,1);
    if( (map_Flags&(1<<4)) > 0 ) {
	    FragColor += vec4(1,0,0,0); // bump
    }
    if( (map_Flags&(1<<5)) > 0 ) {
	    FragColor += vec4(0,1,0,0); // nor
    }
	if( (map_Flags&(1<<0)) > 0 ) {
	    FragColor += vec4(0,0,1,0); // Kd
    }
}
