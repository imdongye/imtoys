#version 410 core
layout(location=0) out vec4 FragColor;

void main(void)
{
	FragColor = vec4(vec3(gl_FragCoord.z),1);
	//FragColor = vec4(1,0,0,1);
}