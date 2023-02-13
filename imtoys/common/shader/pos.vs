#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat= mat4(1);

out vec3 wPos;

void main(void) {
	wPos = (modelMat * vec4(aPos, 1.0)).xyz;
	gl_Position= projMat*viewMat*vec4(wPos,1.0);
}
