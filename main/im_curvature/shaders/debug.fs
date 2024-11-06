#version 410 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 cam_Pos;
uniform float gamma = 2.2;

void main()
{    
    vec3 V = cam_Pos - wPos; 
    V = normalize(V);
    // vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	vec3 N = wNor;

    vec3 outColor = vec3(max(dot(N, V),0));
    
    outColor = pow(outColor, vec3(1/gamma));

    
    FragColor = vec4(outColor, 1);
}
