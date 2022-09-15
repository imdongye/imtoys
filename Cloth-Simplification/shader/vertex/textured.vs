#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec3 vNor;
out vec2 tUv;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat = mat4(1);

void main(void)
{
    wPos = (modelMat * vec4(aPos, 1)).xyz;
	/* do not with non-uniform scale */
    wNor = (modelMat * vec4(aNor, 0)).xyz;
	vNor = (viewMat * vec4(wNor, 0)).xyz;
    tUv = aUv;
    gl_Position = projMat * viewMat * vec4(wPos, 1.0);
}
