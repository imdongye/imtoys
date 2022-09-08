#version 410 core
out vec4 FragColor;

uniform vec4 color = vec4(1);

void main(void)
{
	FragColor = vec4(vec3(gl_FragCoord.z),1);//color;
}