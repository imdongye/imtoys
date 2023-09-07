#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 tUv;

uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;

uniform vec3 cameraPos;
uniform float gamma = 2.2;

void main()
{   
    vec3 outColor = texture(map_Bump0, tUv).xyz;
    //vec3 outColor = texture(map_Kd0, tUv).xyz;
    //vec3 outColor = texture(map_Kd0, tUv).xyz;
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
