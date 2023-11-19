#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);

uniform float cameraAspect;
uniform vec3 camera_Pos;
uniform float cameraFovy;
uniform vec3 cameraPivot;
uniform vec3 light_Pos;
uniform float light_Int;

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
vec3 mat_BaseColor = vec3(1);
float roughness;
float metalness;

vec3 N, L, V, R, H, F0;
float NDL, NDV, NDH, VDR;

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

float getPrimDist(int primType, vec3 mPos) {
    switch(primType)
    {
    case PM_SPHERE:
        return sdSphere(mPos);
    case PM_BOX:
        return sdBox(mPos);
    case PM_PIPE:
        return sdPipe(mPos);
    case PM_DONUT:
        return sdDonut(mPos, 1);
    }
}
float operateDist(int opType, float a, float b, float blendness) {
    switch(opType)
    {
    case OT_ADD_ROUND:
        return fOpUnionRound(a, b, blendness);
    case OT_ADD_EDGE:
        return fOpUnionChamfer(a, b, blendness);
    case OT_SUB_ROUND:
        return fOpDifferenceRound(a, b, blendness);
    case OT_SUB_EDGE:
        return fOpDifferenceChamfer(a, b, blendness);
    case OT_INT_ROUND:
        return fOpIntersectionRound(a, b, blendness);
    case OT_INT_EDGE:
        return fOpIntersectionChamfer(a, b, blendness);
    }
}

float sdWorld(vec3 p) {
    float dist = far_distance; 
    dist = p.y; // plane

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        mat4 transform = transforms[i];
        vec3 mPos = (transform*vec4(p,1)).xyz;
        float primDist = getPrimDist(prim_types[i], mPos);
        float tempDist = operateDist(op_types[i], dist, primDist, blendness);
        dist = tempDist;
    }
    return dist;
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
float updateMaterial(vec3 p) {
    float dist = far_distance; 
    dist = p.y; // plane
    
    mat_BaseColor = vec3(1);
    roughness = 0;
    metalness = 0;

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        mat4 transform = transforms[i];
        vec3 mPos = (transform*vec4(p,1)).xyz;
        float primDist = getPrimDist(prim_types[i], mPos);
        float tempDist = operateDist(op_types[i], dist, primDist, blendness);
       
        // hard part
        int matIdx = mat_idxs[i];
        float percent = dist/(primDist+dist);
        mat_BaseColor = mix(mat_BaseColor, base_colors[matIdx], percent);
        roughness = mix(roughness, roughnesses[matIdx], percent);
        metalness = mix(metalness, metalnesses[matIdx], percent);
       
        dist = tempDist;
    }
    return dist;
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

float distributionGGX() {
    float a = roughness*roughness;
	float aa = a*a;
	float theta = acos(NDH);
	return aa/( PI * pow(cos(theta),4) * pow(aa+pow(tan(theta),2),2) );
}
float geometryCookTorrance() {
    float t1 = 2*NDH*NDV/dot(V,H);
	float t2 = 2*NDH*NDL/dot(V,H);
	return min(1, min(t1, t2));
}
vec3 fresnelSchlick() {
    float theta = NDH;
	// theta = dot(R, w_o);
	return F0+(vec3(1)-F0)*pow(1-cos(theta), 5);
}

vec3 brdfCookTorrence() {
    float D = distributionGGX();
    float G = geometryCookTorrance();
    vec3 F = fresnelSchlick();
    return D*G*F / (4*NDL*NDV);
}

vec3 brdf() {
    
    vec3 diff = mat_BaseColor* mix(1/PI, 0, metalness);
    vec3 spec = brdfCookTorrence();
    return diff + spec;
}


vec3 render(vec3 wPos, vec3 view) {
	vec3 toLight = light_Pos-wPos;
    V = view;
    N = getNormal(wPos);
    L = normalize(toLight);
    H = normalize((V+L)/2.0);
    R = reflect(-L, N);
    F0 = mix(vec3(0.24), mat_BaseColor, metalness);

    NDL = max(0, dot(N,L));
    NDV = max(0, dot(N,V));
    NDH = max(0, dot(N,H));
    VDR = max(0, dot(V,R));

    // return (NDL+pow(VDR, 1000))*mat_BaseColor;

	vec3 Li = light_Int*vec3(1)/dot(toLight,toLight);

    return brdf() * Li * NDL;
}

// gamma correction
void convertLinearToSRGB( inout vec3 rgb ){
    rgb.r = rgb.r<0.0031308?(12.92*rgb.r):(1.055*pow(rgb.r,0.41667)-0.055);
    rgb.g = rgb.g<0.0031308?(12.92*rgb.g):(1.055*pow(rgb.g,0.41667)-0.055);
    rgb.b = rgb.b<0.0031308?(12.92*rgb.b):(1.055*pow(rgb.b,0.41667)-0.055);
}

void main()
{
    vec2 uv = texCoord*2-vec2(1);
    vec3 ro = camera_Pos;
    vec3 camFront = normalize( cameraPivot-camera_Pos );
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
        vec3 wPos = ro + rd*hitDist;
        updateMaterial(wPos);
        outColor = render( wPos, -rd );
        outColor = mat_BaseColor;
    }
    
    // debug
    //outColor = vec3(1,0,0);

    convertLinearToSRGB(outColor);
    FragColor = vec4(outColor, 1);
}  