#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;

uniform vec3 lightDir;
uniform float gamma = 2.2;

void main()
{    
    vec3 L = lightDir;
    L = normalize(L);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNor;

    FragColor = vec4(max(dot(N, L),0)*vec3(1),1);
	FragColor = pow(FragColor, vec4(1/gamma));
}
