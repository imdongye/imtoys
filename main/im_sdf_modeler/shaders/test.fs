#version 410 core
layout (location = 0) out vec4 FragColor;

precision highp float;

in vec2 texCoord;

uniform float cameraAspect;
uniform vec3 cameraPos;
uniform float cameraFovy;
uniform vec3 cameraPivot;

uniform vec3 lightPos;
uniform float lightInt;

uniform mat4 modelMat;


const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);
const int NR_STEPS = 100;
const float MIN_HIT_DIST = 0.01;
const float MAX_FAR_DIST = 100.0;
const float EPSILON_FOR_NORMAL = 0.01;


float sdSphere(vec3 p, vec3 c, float r) {
    return length(p-c) - r;
}
float sdBox( vec3 p, vec3 b ) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
float sdPlane(vec3 p, vec3 n, float h) {
    return dot(n,p) - h;
}
float smoothMin(float a, float b, float k) {
    return -log(exp(-k*a) + exp(-k*b)) / k;
}

float sdWorld(vec3 p) {
    vec4 hp = vec4(p, 1);
    float sphere0 = sdSphere(p, vec3(0,1,0), 1.0);
    float plane = sdPlane(p, UP, 0.0);
    float box = sdBox((modelMat*hp).xyz, vec3(1,0.5,1));
    float rst = sphere0;
    rst = smoothMin(plane, rst, 100.0);
    return rst;
}

vec3 getNormal(vec3 p) {
    const vec2 e = vec2(EPSILON_FOR_NORMAL, 0);
    float dist = sdWorld(p);
    float dDdx = sdWorld(p+e.xyy) - dist;
    float dDdy = sdWorld(p+e.yxy) - dist;
    float dDdz = sdWorld(p+e.yyx) - dist;
    return normalize(vec3(dDdx, dDdy, dDdz));

    // float dDdx = sdWorld(p+e.xyy) - sdWorld(p-e.xyy);
    // float dDdy = sdWorld(p+e.yxy) - sdWorld(p-e.yxy);
    // float dDdz = sdWorld(p+e.yyx) - sdWorld(p-e.yyx);
    // return normalize(vec3(dDdx, dDdy, dDdz));
    // vec3 dPdx = dFdx(p);
    // vec3 dPdy = dFdy(p);
    // return normalize(cross(dPdx, dPdy));
}

float rayMarch(vec3 origin, vec3 dir, float maxDist) {
    float dist = 0;
    for( int i=0; i<NR_STEPS; i++ )
    {
        float closestDist = sdWorld( dist*dir + origin );
        if( closestDist<MIN_HIT_DIST ) {
            break;
        }
        dist += closestDist;
        if( dist>maxDist ) {
            break;
        }
    }
    return dist;
}

float softShadow(vec3 origin, vec3 dir, float maxDist) {
    float shadow = 1;
    float k = 8;
    float marchingDist = 0;
    for( int i=0; i<NR_STEPS; i++ ) {
        float dist = sdWorld(origin+marchingDist*dir);
        shadow = min(shadow, k*dist/marchingDist);
        if( marchingDist>maxDist) {
            break;
        }
    }
    return 0.25*(1+shadow)*(1+shadow)*(2-shadow);
}

void main()
{
    vec2 uv = texCoord*2-vec2(1);
    vec3 camFront = normalize( cameraPivot-cameraPos );
    vec3 camRight = normalize( cross(camFront, UP) );
    vec3 camUp = normalize( cross(camRight, camFront) );
    float eyeZ = 1/tan((PI/360)*cameraFovy);
    vec3 rayDir = normalize( cameraAspect*uv.x*camRight + uv.y*camUp + eyeZ*camFront );
    vec3 P = rayMarch(cameraPos, rayDir);
    vec3 N = getNormal(P);
    vec3 R = reflect(rayDir, N);
    vec3 V = -rayDir;
    vec3 L = lightPos-P;
    float litDist = length(L);
    L /= litDist;
    float ndl = max(0, dot(N,L));
    float vdr = max(0, dot(V,R));
    float specular = pow(vdr, 1000);
    float shadow = softShadow(P, L, litDist);
    float rst = shadow*ndl+specular;
    vec3 outColor = vec3(rst);
    // outColor = P;
    FragColor = vec4(outColor, 1);
}  