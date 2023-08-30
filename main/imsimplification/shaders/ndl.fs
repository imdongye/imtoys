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

    vec3 outColor = max(dot(N, L),0)*vec3(1);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);

}
