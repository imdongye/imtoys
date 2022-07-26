#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

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
    TexCoords = aTexCoords;    
    gl_Position = projMat * viewMat * modelMat * vec4(aPos, 1.0);
}
