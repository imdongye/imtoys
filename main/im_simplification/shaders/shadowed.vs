#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;
out vec4 shadowFragPos;

uniform mat4 model_Mat = mat4(1);
uniform mat4 view_Mat = mat4(1);
uniform mat4 proj_Mat= mat4(1);

uniform mat4 shadow_VP = mat4(1);

void main()
{
    wPos = (model_Mat * vec4(aPos, 1.0)).xyz;
    wNor = (model_Mat * vec4(aNor, 0)).xyz; // do not with nonuniform scale
    mUv = aUv;
    gl_Position = proj_Mat * view_Mat * vec4(wPos, 1.0);

	shadowFragPos = shadow_VP * vec4(wPos, 1.0);
}