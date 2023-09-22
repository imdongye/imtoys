#version 410 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

void main()
{
    wPos = aPos;
    wNor = aNor;
    mUv = aUv;

	vec2 nc = mUv*2-1;
    gl_Position = vec4(nc, 0, 1);
}