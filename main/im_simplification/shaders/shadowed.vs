#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;
out vec4 shadowFragPos;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat= mat4(1);

uniform mat4 shadowVP = mat4(1);

void main()
{
    wPos = (modelMat * vec4(aPos, 1.0)).xyz;
    wNor = (modelMat * vec4(aNor, 0)).xyz; // do not with nonuniform scale
    mUv = aUv;
    gl_Position = projMat * viewMat * vec4(wPos, 1.0);

	shadowFragPos = shadowVP * vec4(wPos, 1.0);
}