#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;

out vec2 texCoord;

void main()
{
    texCoord = aPos*0.5+0.5;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  