#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat= mat4(1);

void main()
{
    wPos = (modelMat * vec4(aPos, 1)).xyz;
    wNor = (modelMat * vec4(aNor, 0)).xyz; // do not with nonuniform scale
    mUv = aUv;

	vec2 nc = mUv*2-1;
    gl_Position = vec4(nc, 0, 1);
}