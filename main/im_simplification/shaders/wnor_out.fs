#version 410 core
layout(location=0) out vec4 fragColor;

const float gamma = 2.2f;

uniform sampler2D map_Bump;

in vec3 wNor;

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 outColor = N*0.5+0.5;
    vec2 uv = gl_FragCoord.xy;
    outColor = texture(map_Bump, uv).xyz;
    outColor = vec3(uv,0);

    outColor = pow(outColor, vec3(1/gamma));
    fragColor = vec4(outColor, 1);
}
