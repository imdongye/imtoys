#version 410 core
layout(location=0) out vec4 fragColor;

const float gamma = 2.2f;

in vec3 wNor;

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 outColor = N*0.5+0.5;

    outColor = pow(outColor, vec3(1/gamma));
    fragColor = vec4(outColor, 1);
}
