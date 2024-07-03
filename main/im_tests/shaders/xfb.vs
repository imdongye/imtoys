#version 460 core

layout(location=0) in vec3 pos;

out vec3 out_norm;
out float out_length;

// bool intersect(vec3 p1, vec3 p2, vec3 t1, vec3 t2, vec3 t3, out vec3 point) {
//     vec3 diff, u, v, n, w0, 2;
//     diff = p2 - p1;
//     u = t2-t1;
//     v = t3-t1;
//     n = cross(u, v);

//     w0 = t1-p1;
//     a = dot(w0, n);
//     b = dot(diff, n);

//     r = a/b;
//     if( r<0.0 || r>1.0 )
//         return false;
    
//     point = p1 + r*diff;
//     w = t1-point;

//     float uu, uv, vv, wu, wv, D;
//     uu = dot(u,u);
//     uv = dot(u,v);
//     vv = dot(v,v);
//     wu = dot(w,u);
//     wv = dot(w,v);
//     D = uv*uv - uu*vv;

//     float s, t;
//     s = (uv*wv - vv*wu) / D;
//     if( s<0.0 || s>1.0 ) 
//         return false;
//     t = (uv*wu - uu*wv) / D;
//     if( t<0.0 || t>1.0 ) 
//         return false;

//     return true;
// }

void main() {
    // out_norm = normalize(pos);
    // out_length = length(pos);
    out_norm = vec3(1,2,3);
    out_length = 10.0;
}