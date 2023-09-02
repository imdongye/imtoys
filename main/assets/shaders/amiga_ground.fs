#version 410 core
out vec4 FragColor;

in vec3 wPos;

uniform vec3 cameraPos;
uniform float gamma = 2.2;

void main()
{
    float d = gl_FragCoord.z;
    float f = mod(floor(wPos.z) + floor(wPos.x), 2.0);
    
    vec3 outColor = mix(vec3(1,1,0),vec3(0,1,0),f);
    //out_color *= mix(vec3(1,1,1),vec3(0.1,0.1,0.1),t/d);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
