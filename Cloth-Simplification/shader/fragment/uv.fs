#version 410 core
out vec4 FragColor;

in vec2 texCoords;
in vec3 wPos;
in vec3 wNorm;
in vec2 tUv;

uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;

uniform vec3 cameraPos;

void main()
{   
    vec3 outColor = vec3(tUv.x, tUv.y, 1);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
