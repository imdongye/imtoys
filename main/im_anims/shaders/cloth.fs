#version 410
layout(location=0) out vec4 FragColor;

in float aM;

void main() {
	vec3 color = mix(vec3(1,0.2,0.2), vec3(0.3,1,0.2), aM);
	FragColor = vec4(color, 1);
}
