#version 410
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

void main() {
	vec3 faceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
    vec3 V = normalize(cameraPos - wPos);
	vec3 L = normalize(lightPos-wPos);
	float lambertian = max(0,dot(N,L));
    float ndv = max(0, dot(N,V));

	vec3 albelo = ( (map_Flags&1) > 0 ) ? texture(map_Kd, mUv).rgb : Kd;
	
    vec3 outColor = albelo*0.5+albelo*vec3(ndv*0.5);
	//outColor = vec3(1.f);

	FragColor = vec4(outColor,1);
}
