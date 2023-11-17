#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);

uniform float cameraAspect;
uniform vec3 cameraPos;
uniform float cameraFovy;
uniform vec3 cameraPivot;
uniform vec3 lightPos;
uniform float lightInt;

const int MAX_MATS = 32;
const int MAX_OBJS = 32;

const int PM_SPHERE = 1;
const int PM_BOX    = 2;
const int PM_PIPE   = 3;
const int PM_DONUT  = 4;

const int OT_ADD_ROUND  = 0;
const int OT_ADD_EDGE   = 1;
const int OT_SUB_ROUND  = 2;
const int OT_SUB_EDGE   = 3;
const int OT_INT_ROUND  = 4;
const int OT_INT_EDGE   = 5;


uniform int nr_march_steps = 100;
uniform float far_distance = 100.0;
uniform float hit_threshold = 0.01;
uniform float diff_for_normal = 0.01;

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

float distToObjs[MAX_OBJS];
vec3 baseColor = vec3(1);
float roughness;
float metalness;

float map01(float min, float max, float x) {
    return (x-min)/(max-min);
}


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
// float smoothMin(float a, float b, float k) {
//     return -log(exp(-k*a) + exp(-k*b)) / k;
// }

/********************************   hg_sdf.glsl part   ********************************/
// https://iquilezles.org/articles/smin/
// The "Chamfer" flavour makes a 45-degree chamfered edge (the diagonal of a square of size <r>):
float fOpUnionChamfer(float a, float b, float r) {
	return min(min(a, b), (a - r + b)*sqrt(0.5));
}
// The "Round" variant uses a quarter-circle to join the two objects smoothly:
float fOpUnionRound(float a, float b, float r) {
	vec2 u = max(vec2(r - a,r - b), vec2(0));
	return max(r, min (a, b)) - length(u);
}
// Intersection has to deal with what is normally the inside of the resulting object
// when using union, which we normally don't care about too much. Thus, intersection
// implementations sometimes differ from union implementations.
float fOpIntersectionChamfer(float a, float b, float r) {
	return max(max(a, b), (a + r + b)*sqrt(0.5));
}
float fOpIntersectionRound(float a, float b, float r) {
	vec2 u = max(vec2(r + a,r + b), vec2(0));
	return min(-r, max (a, b)) + length(u);
}
// Difference can be built from Intersection or Union:
float fOpDifferenceChamfer (float a, float b, float r) {
	return fOpIntersectionChamfer(a, -b, r);
}
float fOpDifferenceRound (float a, float b, float r) {
	return fOpIntersectionRound(a, -b, r);
}

/********************************   hg_sdf.glsl part   ********************************/


float sdWorld(vec3 p)
{
    float dist = far_distance; 
    dist = p.y; // plane

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        mat4 transform = transforms[i];
        vec3 mPos = (transform*vec4(p,1)).xyz;
        float primDist, tempDist;
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
        case OT_ADD_ROUND:
            tempDist = fOpUnionRound(dist, primDist, blendness);
            break;
        case OT_ADD_EDGE:
            tempDist = fOpUnionChamfer(dist, primDist, blendness);
            break;
        case OT_SUB_ROUND:
            tempDist = fOpDifferenceRound(dist, primDist, blendness);
            break;
        case OT_SUB_EDGE:
            tempDist = fOpDifferenceChamfer(dist, primDist, blendness);
            break;
        case OT_INT_ROUND:
            tempDist = fOpIntersectionRound(dist, primDist, blendness);
            break;
        case OT_INT_EDGE:
            tempDist = fOpIntersectionChamfer(dist, primDist, blendness);
            break;
        }

        distToObjs[i] = primDist;
        dist = tempDist;
    }
    return dist;
}

void updateMaterial(vec3 p) 
{
    baseColor = vec3(1);
    roughness = 0;
    metalness = 0;

    float totalDist = p.y;

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        mat4 transform = transforms[i];
        vec3 mPos = (transform*vec4(p,1)).xyz;
       
        float primDist, tempDist;
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
        case OT_ADD_ROUND:
            tempDist = fOpUnionRound(totalDist, primDist, blendness);
            break;
        case OT_ADD_EDGE:
            tempDist = fOpUnionChamfer(totalDist, primDist, blendness);
            break;
        case OT_SUB_ROUND:
            tempDist = fOpDifferenceRound(totalDist, primDist, blendness);
            break;
        case OT_SUB_EDGE:
            tempDist = fOpDifferenceChamfer(totalDist, primDist, blendness);
            break;
        case OT_INT_ROUND:
            tempDist = fOpIntersectionRound(totalDist, primDist, blendness);
            break;
        case OT_INT_EDGE:
            tempDist = fOpIntersectionChamfer(totalDist, primDist, blendness);
            break;
        }
        // hard part
        int matIdx = mat_idxs[i];
        float percent = totalDist/(primDist+totalDist);
        baseColor = mix(baseColor, base_colors[matIdx], percent);
        roughness = mix(roughness, roughnesses[matIdx], percent);
        metalness = mix(metalness, metalnesses[matIdx], percent);

        totalDist = tempDist;
    }
}


vec3 getNormal(vec3 p) {
    vec2 e = vec2(diff_for_normal, 0);
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
    for( int i=0; i<nr_march_steps; i++ )
    {
        float closestDist = sdWorld( dist*dir + origin );
        if( closestDist<hit_threshold ) {
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
    updateMaterial(p);

    vec3 color = baseColor*rst;
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
    float hitDist = rayMarch(ro, rd, far_distance);
    vec3 outColor;
    
    if( hitDist > far_distance ) {
        outColor = vec3(0,0.001,0.3);
    }
    else {
        outColor = render( ro + rd*hitDist, -rd );
    }
    
    // debug
    //outColor = vec3(1,0,0);
    
    FragColor = vec4(outColor, 1);
}  