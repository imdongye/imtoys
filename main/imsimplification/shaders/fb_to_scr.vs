#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;

out vec2 tUv;

void main()
{
    tUv = aUv;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  