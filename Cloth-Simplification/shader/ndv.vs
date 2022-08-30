#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 wPos;
out vec3 wNorm;

uniform mat4 modelMat = mat4(1,0,0,0,
                             0,1,0,0,
                             0,0,1,0,
                             0,0,0,1);
uniform mat4 viewMat = mat4(1,0,0,0,
                            0,1,0,0,
                            0,0,1,0,
                            0,0,0,1);
uniform mat4 projMat= mat4(1,0,0,0,
                           0,1,0,0,
                           0,0,1,0,
                           0,0,0,1);

void main()
{
    wPos = (modelMat * vec4(aPos, 1.0)).xyz;
    wNorm = (modelMat * vec4(aNormal, 1.0)).xyz; // do not with nonuniform scale
    gl_Position = projMat * viewMat * vec4(wPos, 1.0);
}