#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 tUv;

uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;

uniform vec3 cameraPos;
uniform float gamma = 2.2;

void main()
{   
    FragColor = texture(map_Kd0, tUv);
	FragColor = pow(FragColor, vec4(1/gamma));
}