#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wNor;

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 outColor = N*0.5+0.5;

    fragColor = vec4(outColor, 1);
}
