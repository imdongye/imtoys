#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 fragColor;

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
	
    vec3 outColor = ndv;

	FragColor = vec4(outColor,1);
}
