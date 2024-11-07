#version 410 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec3 wCol;

uniform vec3 cam_Pos;
uniform float gamma = 2.2;

vec3 getValueColor(float value) {
    float w_b = 0;
    float w_g = 0;
    float w_r = 0;
    if(value<0) {
        return vec3(0,0,1);
    }
    if(value>1) {
        return vec3(1,0,0);
    }

    if( value < 0.25 ) {
        w_b = 1;
        w_g = value/0.25;
    } else if(value < 0.75) {
        w_b = max(0, 1 - (value-0.25) / 0.25);
        w_g = 1;
        w_r = max(0, (value - 0.5) / 0.25);
    } else {
        w_g = 1 - (value - 0.75)/0.25;
        w_r = 1;
    }

    vec3 rst = w_b*vec3(0,0,1) + w_g*vec3(0,1,0) + w_r*vec3(1,0,0);
    rst /= w_b+w_g+w_r;
    return rst;
}

void main()
{    
    vec3 V = cam_Pos - wPos; 
    V = normalize(V);
    // vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	vec3 N = normalize(wNor);

    vec3 outColor = vec3(max(dot(N, V),0));

    float value = wCol.r;
    value = wPos.x*0.8+0.6;
    outColor *= getValueColor(value);

    // debug
    // outColor = N*0.5+0.5;

    
    FragColor = vec4(pow(outColor, vec3(1/gamma)), 1);
}
