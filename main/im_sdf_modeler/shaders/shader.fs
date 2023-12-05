/*

2023-11-24 / imdongye

operation and primitive function is ref from
    https://iquilezles.org/articles/smin/
    and
    https://mercury.sexy/hg_sdf/
*/

#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;
const vec3 UP = vec3(0,1,0);

uniform float camera_Aspect;
uniform vec3 camera_Pos;
uniform float camera_Fovy;
uniform vec3 camera_Pivot;
uniform vec3 light_Pos;
uniform float light_Int;

const int PT_SPHERE = 1;
const int PT_BOX    = 2;
const int PT_PIPE   = 3;
const int PT_DONUT  = 4;

const int OT_ADD_ROUND  = 0;
const int OT_ADD_EDGE   = 1;
const int OT_SUB_ROUND  = 2;
const int OT_SUB_EDGE   = 3;
const int OT_INT_ROUND  = 4;
const int OT_INT_EDGE   = 5;


const int MAX_MATS = 32;
const int MAX_OBJS = 32;
const int MAX_PRIMS = 32;

uniform int nr_march_steps;
uniform float far_distance;
uniform float hit_threshold;
uniform float diff_for_normal;

uniform vec3 base_colors[MAX_MATS];
uniform float roughnesses[MAX_MATS];
uniform float metalnesses[MAX_MATS];
uniform vec3 sky_color;

uniform int nr_objs;
uniform mat4 transforms[MAX_OBJS];
uniform float scaling_factors[MAX_OBJS];
uniform int mat_idxs[MAX_OBJS];
uniform int prim_types[MAX_OBJS];
uniform int prim_idxs[MAX_OBJS];
uniform int op_types[MAX_OBJS];
uniform float blendnesses[MAX_OBJS];
uniform float roundnesses[MAX_OBJS];

uniform vec2 donuts[MAX_PRIMS];
uniform float capsules[MAX_PRIMS];

float distToObjs[MAX_OBJS];

vec3 N, L, V, R, H;
float NDL, NDV, NDH, VDR, HDV;
vec3 baseColor, F0;
float roughness, metalness;

/************** used marching cube too ***************/
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

float fOpUnionChamfer(float a, float b, float r) {
	return min(min(a, b), (a - r + b)*sqrt(0.5));
}
float fOpUnionRound(float a, float b, float r) {
	vec2 u = max(vec2(r - a,r - b), vec2(0));
	return max(r, min (a, b)) - length(u);
}
float fOpIntersectionChamfer(float a, float b, float r) {
	return max(max(a, b), (a + r + b)*sqrt(0.5));
}
float fOpIntersectionRound(float a, float b, float r) {
	vec2 u = max(vec2(r + a,r + b), vec2(0));
	return min(-r, max (a, b)) + length(u);
}
float fOpDifferenceChamfer (float a, float b, float r) {
	return fOpIntersectionChamfer(a, -b, r);
}
float fOpDifferenceRound (float a, float b, float r) {
	return fOpIntersectionRound(a, -b, r);
}

// model space distance
float getPrimDist(int primType, vec3 mPos) {
    switch(primType)
    {
    case PT_SPHERE:
        return sdSphere(mPos);
    case PT_BOX:
        return sdBox(mPos);
    case PT_PIPE:
        return sdPipe(mPos);
    case PT_DONUT:
        return sdDonut(mPos, 1);
    }
}
float getObjDist(int objIdx, vec3 wPos) {
    mat4 transform = transforms[objIdx];
    vec3 mPos = (transform*vec4(wPos,1)).xyz;
    float primDist = getPrimDist(prim_types[objIdx], mPos);
    return primDist * scaling_factors[objIdx];
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

float sdWorld(vec3 wPos) {
    float dist = far_distance; 
    dist = wPos.y; // plane

    for( int i=0; i<nr_objs; i++ ) 
    {
        float blendness = blendnesses[i];
        float objDist = getObjDist(i, wPos);
        float tempDist = operateDist(op_types[i], dist, objDist, blendness);
        dist = tempDist;
    }
    return dist;
}
/************** ****************** ***************/


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
    
    baseColor = vec3(1);
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
        baseColor = mix(baseColor, base_colors[matIdx], percent);
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

float distributionGGX() { // Todo: 하이라이트 중간 깨짐, 음수
	float a = roughness*roughness;
	float aa = a*a;
	float theta = acos( NDH );
	float num = aa;
	float denom = PI * pow(NDH, 4)* pow(aa+pow(tan(theta),2) ,2);
	return num/max(denom,0.00001);
}
float geometryCookTorrance() { // GGX
	float t1 = 2*NDH*NDV/HDV;
	float t2 = 2*NDH*NDL/HDV;
	return min(1, min(t1, t2));
}
vec3 fresnelSchlick() {
	float cosTheta = NDV; // NDH HDV
	float ratio = max(0,pow(1-cosTheta, 5));
	return mix(F0, vec3(1), ratio);
}

vec3 brdfCookTorrence() {
    float D = distributionGGX();
    float G = geometryCookTorrance();
    vec3 F = fresnelSchlick();
    return D*G*F / (4*NDL*NDV);
}

vec3 brdf() {
    
    vec3 diff = baseColor* mix(1/PI, 0, metalness);
    vec3 spec = brdfCookTorrence();
    return diff + spec;
}


vec3 render(vec3 wPos, vec3 view) {
	vec3 toLight = light_Pos-wPos;
    N = getNormal(wPos);
    V = view;
    L = normalize(toLight);
    R = reflect(-L, N);
    H = normalize(L+V);

    updateMaterial(wPos);
    F0 = mix(vec3(0.24), baseColor, metalness);

    NDL = dot(N,L);
    NDV = dot(N,V);
    NDH = dot(N,H);
    VDR = dot(V,R);
	HDV = dot(V,H);

    // return (NDL+pow(VDR, 1000))*mat_BaseColor;


	vec3 Li = light_Int*vec3(1)/dot(toLight,toLight);
	vec3 ambient = 0.01*1.0*baseColor; // 1.0 is ambOcc

    return brdf() * Li * max(0,NDL) + ambient;
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
    vec3 camFront = normalize( camera_Pivot-camera_Pos );
    vec3 camRight = normalize( cross(camFront, UP) );
    vec3 camUp = normalize( cross(camRight, camFront) );
    float eyeZ = 1/tan((PI/360)*camera_Fovy);
    vec3 rd = normalize( camera_Aspect*uv.x*camRight + uv.y*camUp + eyeZ*camFront );
    float hitDist = rayMarch(ro, rd, far_distance);
    vec3 outColor;
    
    if( hitDist > far_distance ) {
        outColor = vec3(0,0.001,0.3);
    }
    else {
        vec3 wPos = ro + rd*hitDist;
        outColor = render( wPos, -rd );

        // debug
        // outColor = baseColor;

    }

    convertLinearToSRGB(outColor);
    FragColor = vec4(outColor, 1);
}  