#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);
const int NR_STEPS = 100;
const float MIN_HIT_DIST = 0.01;
const float MAX_FAR_DIST = 100.0;
const float EPSILON_FOR_NORMAL = 0.01;

uniform float cameraAspect;
uniform vec3 cameraPos;
uniform float cameraFovy;
uniform vec3 cameraPivot;
uniform vec3 lightPos;
uniform float lightInt;

const int MAX_MATS = 64;
const int MAX_OBJS = 64;

const int PM_SPHERE = 1;
const int PM_BOX    = 2;
const int PM_PIPE   = 3;
const int PM_DONUT  = 4;

const int OT_ADDITION     = 0;
const int OT_SUBTRACTION  = 1;
const int OT_INTERSECTION = 2;

uniform vec3 base_colors[MAX_MATS];
uniform float roughnesses[MAX_MATS];
uniform float metalnesses[MAX_MATS];

uniform int nr_objs;
uniform mat4 transforms[MAX_OBJS];
uniform int mat_idxs[MAX_OBJS];
uniform int prim_types[MAX_OBJS];
uniform int prim_idxs[MAX_OBJS];
uniform int op_types[MAX_OBJS];
uniform float blendnesses[MAX_OBJS];
uniform float roundnesses[MAX_OBJS];



float sdSphere( vec3 p ) {
    return length(p) - 0.5f;
}
float sdBox( vec3 p ) {
  vec3 q = abs(p) - vec3(0.5f);
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
float sdPipe( vec3 p ) {
    float d = length(p.xz) - 0.5f;
	return max(d, abs(p.y) - 1.f);
}
float sdDonut( vec3 p, float r ) {
    vec2 q = vec2(length(p.xz)-r,p.y);
    return length(q);
}
// float sdPlane(vec3 p, vec3 n, float h) {
//     return dot(n,p) - h;
// }
float smoothMin(float a, float b, float k) {
    return min(a,b);
    return -log(exp(-k*a) + exp(-k*b)) / k;
}

float sdWorld(vec3 p)
{
    float dist = p.y; // plane

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        mat4 transform = mat4(1);
        transform = transforms[i];
        vec3 mPos = (transform*vec4(p,1)).xyz;
        float primDist;
        switch(prim_types[i])
        {
        case PM_SPHERE:
            primDist = sdSphere(mPos);
            break;
        case PM_BOX:
            primDist = sdBox(mPos);
            break;
        case PM_PIPE:
            primDist = sdPipe(mPos);
            break;
        case PM_DONUT:
            primDist = sdDonut(mPos, 1);
            break;
        }
        switch(op_types[i])
        {
        case OT_ADDITION:
            dist = smoothMin(dist, primDist, blendness);
            break;
        case OT_SUBTRACTION:
            dist = smoothMin(dist, primDist, blendness);
            break;
        case OT_INTERSECTION:
            dist = smoothMin(dist, primDist, blendness);
            break;
        }
    }
    return dist;
}

// float sdWorld(vec3 p)
// {
//     float dist = p.y; // plane
//     return dist;
// }

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
    float hitDist = rayMarch(ro, rd, MAX_FAR_DIST);
    vec3 outColor;
    if( hitDist > MAX_FAR_DIST ) {
        outColor = vec3(0,0.001,0.3);
    }
    else {
        outColor = render( ro + rd*hitDist, -rd );
    }
    
    // debug
    //outColor = vec3(1,0,0);
    
    FragColor = vec4(outColor, 1);
}  