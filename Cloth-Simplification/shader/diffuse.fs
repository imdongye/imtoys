#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D map_Kd1;

void main()
{    
    FragColor = texture(map_Kd1, TexCoords);
}