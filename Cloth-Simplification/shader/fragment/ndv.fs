#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 tUv;

uniform vec3 cameraPos;
uniform float gamma = 2.2;

void main()
{    
    vec3 V = cameraPos - wPos; 
    V = normalize(V);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNorm;

    FragColor = vec4(max(dot(N, V),0)*vec3(1),1);
	FragColor = pow(FragColor, vec4(1/gamma));
}
