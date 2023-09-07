#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 FragColor;

void main() {
	FragColor = vec4(0.3,0.35,1,1);
}
