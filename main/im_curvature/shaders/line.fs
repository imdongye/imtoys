#version 410 core
layout(location=0) out vec4 FragColor;

uniform vec4 color = vec4(1);

void main()
{
    FragColor = color;
}
