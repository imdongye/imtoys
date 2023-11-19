//
// hex-tiling and edge smoothing 
//
// 2023-2-24 / im dongye from https://github.com/imdongye
//

const vec2 dlo = vec2(1.7320508076, 3); // double line offset
const vec2 nlo = dlo*0.5; // next line offset (length:sqrt3)

vec2 hash2( vec2 p ) { return fract(sin(vec2( 1.0+dot(p,vec2(37.0,17.0)), 
                                              2.0+dot(p,vec2(11.0,47.0))))*103.0); }

// hexagon: width(sqrt3), height(2)
// return: {offset, hexpos}
vec4 calcHexInfo( vec2 uv )
{
    vec4 center = (round( vec4(uv, uv-nlo) / dlo.xyxy )) * dlo.xyxy;
    center.zw += nlo;
    vec4 offset = vec4(uv, uv) - center; //+tileoffset
    return ( dot(offset.xy, offset.xy) < dot(offset.zw, offset.zw) )
           ? vec4( offset.xy, center.xy )
           : vec4( offset.zw, center.zw );
}

float calcHexDistance( vec2 off )
{
    off = abs(off);
    return max(dot(nlo, off)/1.5, off.x/nlo.x);
}

vec3 calcHexSepDistance( vec2 off )
{
    off = abs(off);
    // side, verticaly(inside, outside)
    return vec3( off.x/nlo.x, dot(nlo, off)/1.5, dot(nlo*vec2(-1,1), off)/1.5);
}

vec3 mixEdge(vec3 main, vec3 side, vec3 veri, vec3 vero, vec3 coefs)
{
    float mainCoef = 1.-max(coefs.x,max(coefs.y,coefs.z));
    float totalCoef = mainCoef+coefs.x+coefs.y+coefs.z;
    vec3 rst = mainCoef*main + coefs.x*side + coefs.y*veri + coefs.z*vero;
    rst /= totalCoef;
    //return vec3(mainCoef);
    return rst;
}

vec2 getPatchUv( vec2 puv, vec2 uv, float scale ) 
{
    // read vector field
    vec2 dir = normalize(hash2(puv)-0.5);
    
    mat2 rot = mat2(dir, -dir.y, dir.x);
    
    vec2 offset = uv-puv;
    
    return scale*(rot*offset)+vec2(.5);
}

void mainImage( out vec4 FragColor, in vec2 fragCoord )
{
    const float mixoff = 0.1;
    const float texScale = 2.;
    const float hexRowCount = 5.;
    vec2 uv = fragCoord/iResolution.y*2.*hexRowCount;

    vec4 hi = calcHexInfo(uv);
    vec2 main = hi.zw;
    vec2 side = main + vec2(dlo.x,0)*sign(hi.x);
    vec2 veri = main + nlo*sign(hi.xy);            // vertical inside (right)
    vec2 vero = main + nlo*sign(hi.xy)*vec2(-1,1); // vertical outside(left)
    
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    
    float total = 1./texScale*(1.-mixoff);
    vec3 tex =     textureGrad(iChannel0, getPatchUv(main, uv, total), dx, dy).rgb;
    vec3 sideTex = textureGrad(iChannel0, getPatchUv(side, uv, total), dx, dy).rgb;
    vec3 veriTex = textureGrad(iChannel0, getPatchUv(veri, uv, total), dx, dy).rgb;
    vec3 veroTex = textureGrad(iChannel0, getPatchUv(vero, uv, total), dx, dy).rgb;
    
    vec3 dist = calcHexSepDistance(hi.xy);
    vec3 coefs = 1.-smoothstep(-mixoff, mixoff, vec3(1.)-dist);
    vec3 col =  mixEdge(tex, sideTex, veriTex, veroTex, coefs);
    
    // debug
    //col = vec3(hilo/3.,0)*(1.-step(0.9, dist));
    //col = sideTex*(1.-step(0.9, dist.y));
    //col = texture(iChannel0, uv).rgb;
    //col = coefs;
    //col = sideTex;
    //col = tex;
    
    FragColor = vec4(col,1.0);
}