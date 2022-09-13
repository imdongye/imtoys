#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat= mat4(1);

void main(void) {
	vec4 worldPos4 = modelMat* vec4( aPos, 1.0 );
	gl_Position= projMat*viewMat*worldPos4;
}
