#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 FragColor;

void main() {
	vec3 N = normalize(wNor);

	FragColor = vec4(N,1);
}