#version 410 core
out vec4 FragColor;

in vec3 wNor;

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 outColor = N*0.5+0.5;

    FragColor = vec4(outColor, 1);
}
