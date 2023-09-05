#version 410 core
layout(location=0) out vec4 fragColor;

void main(void)
{
	fragColor = vec4(vec3(gl_FragCoord.z),1);
}