#version 410 core
layout(location=0) out vec4 fragColor;


uniform float gamma = 2.2f;
uniform sampler2D map_Bump;

in vec3 wNor;
in vec2 mUv;

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 outColor = N*0.5+0.5;
    outColor = texture(map_Bump, mUv).xyz;

    outColor = pow(outColor, vec3(1/gamma));
    fragColor = vec4(outColor, 1);
}
