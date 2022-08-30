#version 410 core
out vec4 FragColor;

in vec2 texCoords;
in vec3 wPos;
in vec3 wNorm;

uniform vec3 cameraPos;

void main()
{    
    vec3 V = cameraPos - wPos; 
    V = normalize(V);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNorm;

    FragColor = vec4(dot(N, V)*vec3(1),1);
}