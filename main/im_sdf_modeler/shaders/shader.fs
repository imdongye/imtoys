#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

uniform float cameraAspect;
uniform vec3 cameraPos;
uniform float cameraFovy;
uniform vec3 cameraPivot;

const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);


float sdSphere(vec3 p, vec3 c, float r) {
    return length(p-c) - r;
}
/// http://www.songho.ca/math/plane/plane.html
float sdPlane(vec3 p, vec3 n, float centerHeight) {
    return dot(p,n) + centerHeight;
}

float smoothMax(float a, float b, float k) {
    return log(exp(k*a) + exp(k*b)) / k;
}
float smoothMin(float a, float b, float k) {
    return -log(exp(-k*a) + exp(-k*b)) / k;

}

float sdWorld(vec3 p) {
    float sphere0 = sdSphere(p, vec3(0,1,0), 1.0);
    float plane = sdPlane(p, UP, 1);
    float rst = min(sphere0, plane);
   // return sphere0;
    return rst;
}

vec3 getNormal(vec3 p) {
    const vec2 e = vec2(.01, 0);
    float dDdx = sdWorld(p+e.xyy) - sdWorld(p-e.xyy);
    float dDdy = sdWorld(p+e.yxy) - sdWorld(p-e.yxy);
    float dDdz = sdWorld(p+e.yyx) - sdWorld(p-e.yyx);
    return normalize(vec3(dDdx, dDdy, dDdz));
}

vec3 rayMarch(vec3 ro, vec3 rd) {
    const int NR_STEPS = 32;
    const float MIN_HIT_DIST = 0.001;
    const float MAX_FAR_DIST = 1000.0;
    float traveldDist = 0.0;

    for( int i=0; i<NR_STEPS; i++ ) {
        vec3 curPos = traveldDist*rd + ro;
        float closestDist = sdWorld(curPos);

        if( closestDist < MIN_HIT_DIST ) {
            return getNormal(curPos)*0.5+0.5;
        }
        if( traveldDist > MAX_FAR_DIST) {
            break;
        }
        traveldDist += closestDist;
    }
    return vec3(0);
}

void main()
{
    vec2 uv = texCoord*2-vec2(1);
    vec3 ro = cameraPos;
    vec3 front = normalize( cameraPivot-cameraPos );
    vec3 right = normalize( cross(front, UP) );
    float eyeZ = 1*tan(PI*cameraFovy/360);

    vec3 rd = uv.x*right*cameraAspect + uv.y*UP + eyeZ*front;

    ro = vec3(0,0,5);
    rd = vec3(uv, -1);

    vec3 outColor = rayMarch(ro, rd);
    
    FragColor = vec4(outColor, 1);
}  