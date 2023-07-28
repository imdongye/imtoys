#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 tUv;

uniform mat4 modelMat = mat4(1);
uniform mat4 viewMat = mat4(1);
uniform mat4 projMat= mat4(1);

void main()
{
    wPos = (modelMat * vec4(aPos, 1.0)).xyz;
    wNor = (modelMat * vec4(aNor, 0)).xyz; // do not with nonuniform scale
    tUv = aUv;

	vec4 nc;
	nc.xy = tUv*2-1;
	nc.z = 0;
	nc.w = 1;
    gl_Position = nc;
}