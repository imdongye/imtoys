#version 410
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

uniform vec3 baseColor;
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


uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

void main() {
	vec3 faceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
    vec3 V = normalize(cameraPos - wPos);
	vec3 L = normalize(lightPos-wPos);
	float lambertian = max(0,dot(N,L));
    float ndv = max(0, dot(N,V));

	vec3 albelo = ( (map_Flags&MF_BASE_COLOR) > 0 ) ? texture(map_BaseColor, mUv).rgb : baseColor;
	
    vec3 outColor = albelo*0.5+albelo*vec3(ndv*0.5);
	//outColor = vec3(1.f);

	FragColor = vec4(outColor,1);
}
