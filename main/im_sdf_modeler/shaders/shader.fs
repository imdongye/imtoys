#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

// struct Primitive {

// };

// struct Material {

// };

// struct Node {
//     Node[]
// };

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
const float MAX_FAR_DIST = 40.0;


float sdSphere(vec3 p, vec3 c, float r) {
    return length(p-c) - r;
}
float sdBox( vec3 p, vec3 b ) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
// h is distance form origin
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
    rst = smoothMin(plane, rst, 1.0);
    rst = min(box, rst);
    //rst = smoothMin(sphere0, plane, 1.0);
    // return plane;
    //return sphere0;
    return rst;
}

vec3 getNormal(vec3 p) {
    const vec2 e = vec2(.01, 0);
    float dDdx = sdWorld(p+e.xyy) - sdWorld(p-e.xyy);
    float dDdy = sdWorld(p+e.yxy) - sdWorld(p-e.yxy);
    float dDdz = sdWorld(p+e.yyx) - sdWorld(p-e.yyx);
    return normalize(vec3(dDdx, dDdy, dDdz));
    // vec3 dPdx = dFdx(p);
    // vec3 dPdy = dFdy(p);
    // return normalize(cross(dPdx, dPdy));
}

float rayMarch(vec3 ro, vec3 rd) {    
    float dist = 0;

    for( int i=0; i<NR_STEPS; i++ )
    {
        vec3 curPos = dist*rd + ro;
        float closestDist = sdWorld(curPos);
        if( closestDist<MIN_HIT_DIST )
            return dist;
        dist += closestDist;
        if( dist>MAX_FAR_DIST )
            break;
    }
    return MAX_FAR_DIST+1;
}

vec3 render(vec3 p, vec3 v) {
    vec3 n = getNormal(p);
    vec3 l = normalize(lightPos - p);
    vec3 r = reflect(-l, n);
    //r = normalize( 2*dot(n, l)*n - l );
    float ndl = max(0, dot(n,l));
    float vdr = max(0, dot(v,r));
    float specular = pow(vdr, 1000);
    float rst = ndl+specular;

    //rst = specular;
    vec3 color = vec3(rst);
    return color;
}

// tan(fovy/2) = 1/eyeZ
// eyeZ = 1 / tan(fovy/2) 

void main()
{
    vec2 uv = texCoord*2-vec2(1);
    vec3 ro = cameraPos;
    vec3 camFront = normalize( cameraPivot-cameraPos );
    vec3 camRight = normalize( cross(camFront, UP) );
    vec3 camUp = normalize( cross(camRight, camFront) );
    float eyeZ = 1/tan((PI/360)*cameraFovy);
    vec3 rd = normalize( cameraAspect*uv.x*camRight + uv.y*camUp + eyeZ*camFront );
    float hitDist = rayMarch(ro, rd);
    vec3 outColor;
    if( hitDist > MAX_FAR_DIST ) {
        outColor = vec3(0,0.001,0.3);
    }
    else {
        outColor = render( ro + rd*hitDist, -rd );
    }
    
    FragColor = vec4(outColor, 1);
}  