#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 fragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

uniform int map_Flags;
uniform vec4 Kd;
uniform sampler2D map_Kd;

void main() {
	vec3 faceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
    vec3 V = normalize(cameraPos - wPos);
	vec3 L = normalize(lightPos-wPos);
	float lambertian = max(0,dot(N,L));
    float ndv = max(0, dot(N,V));

	vec4 albelo = ( (map_Flags&1) > 0 ) ? texture(map_Kd, mUv) : Kd;
	
    vec3 outColor = albelo.rgb*0.5+albelo.rgb*vec3(ndv*0.5);
	//outColor = vec3(1.f);

	fragColor = vec4(outColor,1);
}
