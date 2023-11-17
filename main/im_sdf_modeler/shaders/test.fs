// SHARED CONSTANTS

const float VERTICAL_FOV = 60.f;

// How far away from the surface counts as 0.
const float MARCH_EPS = 0.00001;

// How far to move away from the surface of the shape before marching reflections and shadows so the march doesn't get stuck near the surface.
const float T_EPS = MARCH_EPS * 2.;
const float NORMAL_EPS = MARCH_EPS * 2.;

// How large of a step to use when computing the gradient.
const float GRADIENT_EPS = 0.001;

// How low of a transmittance to use as a cutoff for reflections.
const float REFLECTION_EPS = 0.001;

const float FAR_CLIP = 100.;
const int MAX_MARCH_STEPS = 256;

// Maximum number of reflections. Total number of casts = 1+MAX_REFLECTIONS.
const int MAX_REFLECTIONS = 5;

const float PI = 3.1416;

// CAMERA MATH

vec3 computeCameraRay (vec3 eye, vec3 target, vec2 uv) {
    vec3 look = target - eye;
    float lookLen = length(look);
    vec3 xDir = normalize(cross(look, vec3(0, 1, 0)));
    vec3 yDir = normalize(cross(xDir, look));
    
    // Distance to move in world-space to move one unit in screen-space.
    float unitDist = tan(VERTICAL_FOV/2. * PI/180.) * lookLen;
    
    vec3 rayTarget = target + unitDist*(xDir*uv.x + yDir*uv.y);
    return normalize(rayTarget - eye);
}

// MATERIALS

// An earlier version represented materials in SdResult as a vector of material weights, with one component for each material in the scene.
// Storing/blending material properties directly scales better for scenes with many textures.
struct Mat {
    vec3 diffuseCol;
    vec3 specularCol;
    float shininess;
    float reflectivity;
};

const Mat red = Mat(vec3(0.2, 0.02, 0.02), vec3(0.04, 0.02, 0.02), 32.0, 0.0);
const Mat green = Mat(vec3(0.02, 0.2, 0.02), vec3(0.02, 0.04, 0.02), 32.0, 0.0);
const Mat blue = Mat(vec3(0.02, 0.02, 0.2), vec3(0.02, 0.02, 0.04), 32.0, 0.0);
const Mat mirror = Mat(vec3(0.01), vec3(0.09), 64., 0.9);

Mat floorMat(vec3 pos) {
    vec3 white = vec3(0.3);
    vec3 black = vec3(0.025);
    
    // Smoothstep for antialising; smooth more strongly further out.
    // TODO: Antialias better. (Idea: Compute screen-space distance (in pixels?) to the other color and blend using that.)
    float smoothstepSize = 0.005;
    float scale = max(10., pow(length(pos), 1.3));
    vec2 tile2D = smoothstep(-smoothstepSize, smoothstepSize, sin(pos.xz * PI) / scale);
    float tile = min(max(tile2D.x, tile2D.y), max(1.-tile2D.x,1.-tile2D.y)); // Fuzzy xor.
    vec3 color = mix(white, black, tile);
    
    return Mat(color,vec3(0.03), 128.0, 0.0);
}

// SCENE DEFINITION

struct SdResult {
    float dist;
    Mat mat;
};

float sdSphere(float r, vec3 p) {
    return length(p) - r;
}

float sdPlane(float height, vec3 p) {
    return p.y - height;
}

float largest(vec3 a) {
    return max(a.x, max(a.y, a.z));
}

float sdBox(vec2 r, vec2 p) {
    vec2 d = abs(p) - r;
    float exterior = length(max(d, 0.));
    float interior = min(max(d.x,d.y), 0.);
    return exterior + interior;
}

float sdBox(vec3 r, vec3 p) {
    vec3 d = abs(p) - r;
    float exterior = length(max(d, 0.));
    float interior = min(largest(d), 0.);
    return exterior + interior;
}

float sdCylinder(vec2 r, vec3 p) {
    return sdBox(r, vec2(length(p.xz), p.y));
}

SdResult sdUnion(SdResult a, SdResult b) {
    if (a.dist < b.dist) return a; else return b;
}

Mat blend(Mat a, Mat b, float k) {
    return Mat(
        mix(a.diffuseCol, b.diffuseCol, k),
        mix(a.specularCol, b.specularCol, k),
        mix(a.shininess, b.shininess, k),
        mix(a.reflectivity, b.reflectivity, k)
    );
}

// TODO: Generalize exponential smoothmin to work with multiple terms, each with their own smoothing coefficient.
// (Start by deriving an asymmetrical smoothmin for two terms, then generalize it to n terms.)

// k.x is the factor used for blending shape; ky is the factor for blending material.
SdResult sminCubic(SdResult a, SdResult b, vec2 k) {
    k = max(k, 0.0001);
    vec2 h = max(k - abs(a.dist - b.dist), 0.0)/k;
    vec2 m = h * h * h * 0.5;
    vec2 s = m * k * (1.0 / 3.0);
    
    SdResult res;
    bool aCloser = a.dist < b.dist;
    res.dist = (aCloser ? a.dist : b.dist) - s.x;
    float blendCoeff = aCloser ? m.y : 1.0-m.y;
    
    res.mat = blend(a.mat, b.mat, blendCoeff);
    return res;
}

SdResult sminCubic(SdResult a, SdResult b, float k) {
    return sminCubic (a, b, vec2(k));
}

vec3 rot45(vec3 p) {
    const float ROOT_2_OVER_2 = 0.70710678118;
    return vec3(ROOT_2_OVER_2 * (p.x - p.z), p.y, ROOT_2_OVER_2 * (p.x + p.z));
}

// I'm reasonably certain that the material calculations get optimized away by the compiler in cases where only the distance is used.
// If this isn't the case, the optimization could be done manually by making a version of this function that only computes the distance.
SdResult sdScene(vec3 p) {
    float time = iTime - 2.8; // Add an offset to the current time so the thumbnail at time 0 looks good.
    vec3 sphere1Pos = vec3(0, 0.5, 0);
    SdResult sphere1 = SdResult(sdSphere(1.0, p - sphere1Pos), red);
    SdResult result = sphere1;
    
    vec3 sphere2Pos = vec3(2.5*cos(time), 1, 2.5*sin(time));
    SdResult sphere2 = SdResult(sdSphere (1.5, p - sphere2Pos), green);
    result = sminCubic(result, sphere2, vec2(2, 1));
    
    vec3 sphere3Pos = vec3(-2.*cos(time/2.), 1, 2.*sin(time/2.));
    SdResult sphere3 = SdResult(sdSphere (1.25, p - sphere3Pos), blue);
    result = sminCubic(result, sphere3, 1.);
    
    SdResult floor = SdResult(sdPlane(0.0, p), floorMat(p));
    result = sminCubic(result, floor, 1.5);
    
    vec3 smallMirrorPos = vec3(1.2, 1.4, 0.);
    SdResult smallMirror = SdResult(sdBox(vec3(0.5), p - smallMirrorPos) - 0.05, mirror);
    result = sminCubic(result, smallMirror, vec2(1., 0.5));
    
    vec3 cylinderPos = vec3(-5., 1., -3.);
    SdResult cylinder = SdResult(sdCylinder(vec2(1., 1.5), p - cylinderPos) - 0.5, mirror);
    
    vec3 boxPos = vec3(5., 0., -4.);
    SdResult box = SdResult(sdBox(vec3(1., 3., 3.), rot45(p - boxPos)) - 0.1, mirror);
    
    SdResult mirrors = sdUnion(cylinder, box);
    result = sminCubic(result, mirrors, 0.1);
    
    return result;
}

// RAY MARCHING

// Returns the distance to the intersection with the scene, or -1 if no intersection is found.
float march(vec3 ro, vec3 rd) {
    float t = T_EPS;
    for (int i = 0; i < MAX_MARCH_STEPS; i++) {
        vec3 pos = ro + rd*t;
        float d = sdScene(pos).dist;
        t += d;
        if (d/t < MARCH_EPS) break;
        if (t > FAR_CLIP) return -1.;
    }
    return t;
}

// Returns a coefficient for how much light makes it to a point from a light source.
// lightApparentSize controls soft shadows. A bigger number means the light takes up a larger viewing area, making softer shadows.
// (If you want to specify an exact apparent radius, pass in tan(radius), or tan(diameter/2.) for an apparent diameter.)
// lightDist is the distance to the light source. Used to prevent marching past a point light.
float shadowMarch(vec3 ro, vec3 norm, vec3 rd, float lightApparentSize, float lightDist) {
    ro += norm * NORMAL_EPS;
    float t = T_EPS;
    
    float minDist = lightApparentSize;
    for (int i = 0; i < MAX_MARCH_STEPS; i++) {
        vec3 pos = ro + rd*t;
        float d = sdScene(pos).dist;
        minDist = min(minDist, d/t);
        t += d;
        if (d < MARCH_EPS) break; // Good for efficiency and shouldn't impact the final result.
        if (minDist/lightApparentSize < MARCH_EPS || t > FAR_CLIP) return max(minDist, 0.) / lightApparentSize;
    }
    // Assume we're in shadow if we didn't complete the march. This is usually a sign that we couldn't get off the original
    // surface because the light vector was close to perpendicular.
    return 0.;
}

// Overload for light sources infinitely far away.
float shadowMarch(vec3 ro, vec3 norm, vec3 rd, float lightApparentSize) {
    return shadowMarch(ro, norm, rd, lightApparentSize, FAR_CLIP);
}

vec3 grad(vec3 pos) {
    const float eps = GRADIENT_EPS;
    // Tetrahedron approach from https://iquilezles.org/articles/normalsSDF.
    vec3 n = vec3(0.0);
    for(int i=min(iFrame,0); i<4; i++) {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*sdScene(pos+eps*e).dist;
    }
    return n;
}

vec3 norm(vec3 pos) {
    return normalize(grad(pos));
}

// LIGHTING

vec3 distantPointLight(vec3 lightDir, vec3 lightCol, float lightApparentSize, Mat mat, vec3 pos, vec3 rd, vec3 norm) {
    vec3 halfVec = normalize(lightDir - rd);
    
    vec3 diffuse = mat.diffuseCol * clamp(dot(norm, lightDir), 0., 1.);
    vec3 specular = mat.specularCol * pow(clamp(dot(norm, halfVec), 0., 1.), mat.shininess);
    // Normalization from http://www.thetenthplanet.de/archives/255.
    float specNormalization = (mat.shininess  + 2.) / (4. * (2. - pow(2., -mat.shininess/2.)));
    
    return lightCol * shadowMarch(pos, norm, lightDir, lightApparentSize) * (diffuse + specular*specNormalization);
}

vec3 light(Mat mat, vec3 pos, vec3 rd, vec3 norm) {
    float occlusion = 1.; // TODO: Some kind of occlusion.
    
    vec3 res = vec3(0);
    
    vec3 sunDir = normalize(vec3(1,0.75,0.5));
    vec3 sunCol = vec3(2);
    float sunApparentSize = 0.047; // The physically correct number is 0.0047, but softer shadows are fun.
    res += distantPointLight(sunDir, sunCol, sunApparentSize, mat, pos, rd, norm);
    
    vec3 ambient =  vec3(0.05) * occlusion;
    res += ambient * mat.diffuseCol;
    
    return res;
}


// RENDERING

vec3 render(vec3 ro, vec3 rd) {
    vec3 color = vec3(0);
    float transmittance = 1.;
    
    for (int i = min(iFrame,0); i <= MAX_REFLECTIONS; i++) {
        float dist = march(ro, rd);
        if (dist > 0.) {
            vec3 pos = ro + dist*rd;
            SdResult sd = sdScene(pos);
            vec3 norm = norm(pos);

            color += transmittance * light(sd.mat, pos, rd, norm);
            transmittance *= sd.mat.reflectivity;
            ro = pos + norm * NORMAL_EPS;
            rd = reflect(rd, norm);
            
            if (transmittance < REFLECTION_EPS) break;

            // TODO: Render light sources as a specular reflection directly on the camera.
        } else {
            vec3 skyLight = vec3(0.4, 0.4, 0.8);
            vec3 skyDark = vec3(0.1, 0.1, 0.4);
            vec3 skyColor = mix(skyDark, skyLight, rd.y);
            color += transmittance * skyColor;
            break;
        }
    }
    
    return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates. (y goes from -1 to 1, x has the same scale per pixel and is centered at 0.)
    vec2 uv = (2.*fragCoord - iResolution.xy)/iResolution.yy;
    
    // Fixed viewpoint.
    // vec3 ro = vec3(0, 4, 4);
    // vec3 target = vec3(0, 0, -2);
    
    // Rotating viewpoint.
    float viewAngle = iTime/10.;
    vec3 ro = 4. * vec3(sin(viewAngle), 1, cos(viewAngle));
    vec3 target = -2. * vec3(sin(viewAngle), 0, cos(viewAngle));
    
    vec3 rd = computeCameraRay(ro, target, uv);
    
    vec3 color = render(ro, rd);
    
    color = pow(color, vec3(0.4545)); // Gamma correction.
    fragColor = vec4(color, 1);
}