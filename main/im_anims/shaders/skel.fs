#version 410 core
layout(location=0) out vec4 FragColor;

flat in ivec4 oBoneIds;
in vec4 oBoneWieghts;

uniform int display_BoneIdx;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 cam_Pos;
uniform float gamma = 2.2;

void main()
{    
    vec3 V = cam_Pos - wPos; 
    V = normalize(V);
    vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	//vec3 N = wNor;

    vec3 outColor = vec3(max(dot(N, V),0));

    for( int i=0; i<4; i++ ) {
        if( oBoneIds[i] == display_BoneIdx ) {
            if( oBoneWieghts[i] >= 0.7 )
                outColor = vec3(1,0,0)*oBoneWieghts[i];
            else if( oBoneWieghts[i] >= 0.6 )
                outColor = vec3(0,1,0)*oBoneWieghts[i];
            else if( oBoneWieghts[i] >= 0.1 )
                outColor = vec3(0,0,1)*oBoneWieghts[i];
            break;  
        }
    }

    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
