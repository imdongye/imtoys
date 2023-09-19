#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wPos;
in vec3 wNor;

uniform vec3 lightPos;
uniform float gamma = 2.2;

void main()
{    
    vec3 L = normalize(lightPos - wPos);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNor;

    vec3 outColor = vec3(max(dot(N, L),0));
    
    outColor = pow(outColor, vec3(1/gamma));
    fragColor = vec4(outColor, 1);
}
