#version 410 core
out vec4 FragColor;

uniform vec4 color = vec4(1);

void main(void)
{
	FragColor = color;
}