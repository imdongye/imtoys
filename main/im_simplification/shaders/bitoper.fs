#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

const float PI = 3.1415926535;
const int MF_NONE       = 0;
const int MF_BASE_COLOR = 1<<0;
const int MF_SPECULAR   = 1<<1;
const int MF_HEIGHT     = 1<<2;
const int MF_NOR        = 1<<3;
const int MF_AMB_OCC    = 1<<4;
const int MF_ROUGHNESS  = 1<<5;
const int MF_METALNESS  = 1<<6;
const int MF_EMISSION   = 1<<7;
const int MF_Opacity    = 1<<8;
const int MF_MR         = 1<<9;
const int MF_ARM        = 1<<1;
const int MF_SHININESS  = 1<<1;

uniform vec3 mat_BaseColor;
uniform vec3 specColor;
uniform vec3 ambientColor;
uniform vec3 emissionColor;

uniform float transmission;
uniform float refraciti;
uniform float opacity;
uniform float shininess;
uniform float roughness;
uniform float metalness;
uniform vec3 f0 = vec3(1);

uniform int map_Flags;

uniform sampler2D map_BaseColor;
uniform sampler2D map_Specular;
uniform sampler2D map_Bump;
uniform sampler2D map_AmbOcc;
uniform sampler2D map_Roughness;
uniform sampler2D map_Metalness;
uniform sampler2D map_Emission;
uniform sampler2D map_Opacity;

uniform float texDelta;
uniform float bumpHeight;


uniform vec3 light_Pos;
uniform vec3 light_Color;
uniform float light_Int;
uniform vec3 camera_Pos;


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
