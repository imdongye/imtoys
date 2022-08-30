#version 410 core
out vec4 FragColor;

in vec2 texCoords;
in vec3 wPos;
in vec3 wNorm;

uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;

uniform vec3 cameraPos;

void main()
{   
    FragColor = vec4(texCoords.x, texCoords.y, 1, 1);
}