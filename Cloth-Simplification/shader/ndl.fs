#version 410 core
out vec4 FragColor;

in vec2 texCoords;
in vec3 wPos;
in vec3 wNorm;

uniform vec3 lightPos;

void main()
{    
    vec3 L = lightPos - wPos; 
    L = normalize(L);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNorm;

    FragColor = vec4(dot(N, L)*vec3(1),1);
}