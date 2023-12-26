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

const int PT_SPHERE = 0;
const int PT_BOX    = 1;
const int PT_PIPE   = 2;
const int PT_DONUT  = 3;

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

uniform float donuts[MAX_PRIMS];
uniform float capsules[MAX_PRIMS];

uniform bool use_IBL = false;
uniform sampler2D map_Light;
uniform sampler2D map_Irradiance;
uniform sampler3D map_PreFilteredEnv;
uniform sampler2D map_PreFilteredBRDF;



vec3 wPos, N, L, V, Rl, H, Rv;
float NDL, NDV, NDH, VDRl, HDV;
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
	return max(d, abs(p.y) - 0.5f);
}
float sdDonut( vec3 p, float r ) {
    vec2 q = vec2(length(p.xz)-r,p.y);
    return length(q)-0.5f;
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
float getObjDist(int objIdx, vec3 wPos) {
    mat4 transform = transforms[objIdx];
    vec3 mPos = (transform*vec4(wPos,1)).xyz;
    float minScale = scaling_factors[objIdx];
    int primIdx = prim_idxs[objIdx];
    float primDist = 0;
    switch(prim_types[objIdx])
    {
    case PT_SPHERE: return minScale * sdSphere(mPos);
    case PT_BOX:    return minScale * sdBox(mPos);
    case PT_PIPE:   return minScale * sdPipe(mPos);
    case PT_DONUT:
        return minScale * sdDonut(mPos, donuts[prim_idxs[objIdx]]);  
    }
    return 0.0;
}
float operateDist(int opType, float a, float b, float blendness) {
    switch(opType)
    {
    case OT_ADD_ROUND:  return fOpUnionRound(a, b, blendness);
    case OT_ADD_EDGE:   return fOpUnionChamfer(a, b, blendness);
    case OT_SUB_ROUND:  return fOpDifferenceRound(a, b, blendness);
    case OT_SUB_EDGE:   return fOpDifferenceChamfer(a, b, blendness);
    case OT_INT_ROUND:  return fOpIntersectionRound(a, b, blendness);
    case OT_INT_EDGE:   return fOpIntersectionChamfer(a, b, blendness);
    }
    return 0.0;
}

float sdWorld(vec3 wPos) {
    float dist = far_distance; 
    // dist = wPos.y; // plane

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
float updateMaterial(vec3 wPos) {
    float dist = far_distance; 
    // dist = wPos.y; // plane
    
    baseColor = vec3(1);
    roughness = 0;
    metalness = 0;

    for( int i=0; i<nr_objs; i++ ) 
    {
        float objDist = getObjDist(i, wPos);
        float tempDist = operateDist(op_types[i], dist, objDist, blendnesses[i]);
       
        // hard part
        int matIdx = mat_idxs[i];
        float percent = dist/(objDist+dist);
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









vec2 uvFromDir(vec3 v) {
	float theta = atan(v.z, v.x); // [0,2PI]->[0,1] z축이 반대방향이라 시계방향으로 뒤집힘
	float phi = asin(v.y);  	  // [1,-1]->[PI/2,-PI/2]->[0,PI]->[1,0]
    vec2 uv = vec2(theta/(2*PI), phi/PI+0.5);
    return uv;
}
vec3 dirFromUv(vec2 uv) {
	float theta = 2*PI*(uv.x);// [0,1]->[0,2PI] 
	float phi = PI*(1-uv.y);  // [0,1]->[PI,0]
	return vec3( cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi) );
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


vec3 pointLighting() {
    vec3 toLight = light_Pos-wPos;
    L = normalize(toLight);
    Rl = reflect(-L, N);
    H = normalize(L+V);

    NDL = dot(N,L);
    NDV = dot(N,V);
    NDH = dot(N,H);
    VDRl = dot(V,Rl);
	HDV = dot(V,H);

    // return (NDL+pow(VDR, 1000))*mat_BaseColor;

	vec3 Li = light_Int*vec3(1)/dot(toLight,toLight);
	vec3 ambient = 0.01*1.0*baseColor; // 1.0 is ambOcc

    return brdf() * Li * max(0,NDL) + ambient;
}

vec3 ibPrefilteredLighting() {
    Rv = reflect(-V, N);
    vec2 uvIrr = uvFromDir(N);
	vec2 uvPfenv = uvFromDir(Rv);
    vec3 diff = mix(baseColor,vec3(0), metalness) * texture(map_Irradiance, uvIrr).rgb  * light_Int*0.01;
	vec3 spec = texture(map_PreFilteredEnv, vec3(uvPfenv,roughness)).rgb * light_Int*0.01;
	vec2 brdf = texture(map_PreFilteredBRDF, vec2(NDV, roughness)).rg;
	spec = spec*(F0*brdf.x + brdf.y);
	return diff + spec;
}
float ao() {
    float dist = 0.07;
    float occ = 1.0;
    for (int i = 0; i < 8; ++i) {
        occ = min(occ, sdWorld( wPos + dist * N) / dist);
        dist *= 2.0;
    }
    occ = max(occ, 0.0);
    return occ*0.7+0.3;
}
vec3 render() {
    N = getNormal(wPos);
    updateMaterial(wPos);
    F0 = mix(vec3(0.24), baseColor, metalness);

	if(use_IBL) {
        return ibPrefilteredLighting()*ao();
    }
    else {
        return pointLighting()*ao();
    }
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
    V = -rd;
    
    if( hitDist > far_distance ) {
        if(use_IBL) {
            vec2 uv = uvFromDir(-V);
            outColor = texture(map_Light, uv).rgb;
        }
        else {
            outColor = sky_color;
        }
    }
    else {
        wPos = ro + rd*hitDist;
        outColor = render();

        // debug
        // outColor = baseColor;

    }

    convertLinearToSRGB(outColor);
    FragColor = vec4(outColor, 1);
}  