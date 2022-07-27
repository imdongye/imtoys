#version 330 core
out vec4 FragColor;

in vec2 texCoords;
in vec3 wPos;
in vec3 wNorm;

uniform sampler2D map_Kd1;

uniform vec3 cameraPos;

void main()
{    
    vec3 V = cameraPos - wPos; 
    V = normalize(V);
    vec3 N = normalize(wNorm);

    FragColor = vec4(dot(N, V)*vec3(1),1);   //texture(map_Kd1, TexCoords);
}