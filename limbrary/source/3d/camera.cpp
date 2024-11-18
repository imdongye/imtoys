//
//	멤버변수로 view projection 생성
//
//  2022-07-21 / im dongye
//
//  note:
//	yaw, pitch, roll 은 free/pivot 모드일때 다르게 사용됨.
//	입력은 degree로 사용하고 행렬 만들때 radian으로 변환함.
//
//  Euler angles : roll(z)-pitch(x)-yaw(y)
//
//	free view (pos and dir) 와 pivot view (track ball, orbit) 지원
//
//  TODO list :
//  1. quaternion
//

#include <limbrary/3d/camera.h>
#include <limbrary/tools/log.h>

using namespace glm;
using namespace lim;


Camera::Camera()
{
	updateViewMtx();
	updateProjMtx();
}
vec3 Camera::screenPosToDir(const vec2& uv) const {
	// vec4 ndcP = vec4(2.f*uv.x-1, 2.f*uv.y-1, -1, 0);
	// // vec4 ndcP = vec4(0.f, 0.f, -1, 0);
	// vec3 wPos = inverse(mtx_Proj*mtx_View) * ndcP;
	vec2 ndc = 2.f*vec2(uv.x, 1.f-uv.y) - vec2(1);
	ndc.x *= aspect;
    float eyeZ = 1.f/tan(radians(fovy/2.f));
    vec3 rd = normalize( ndc.x*right + ndc.y*up + eyeZ*front );
	return rd;
}
void Camera::moveShift(const glm::vec3& off) {
	pos += off;
	pivot += off;
}
void Camera::updateViewMtx()
{
	to_pivot = pivot - pos;
	distance = glm::length(to_pivot);
	front = normalize(to_pivot);
	right = normalize(cross(front, global_up));
	up = cross(right, front);

	mtx_View[0][0] = right.x;
	mtx_View[1][0] = right.y;
	mtx_View[2][0] = right.z;
	mtx_View[3][0] =-dot(right, pos);
	mtx_View[0][1] = up.x;
	mtx_View[1][1] = up.y;
	mtx_View[2][1] = up.z;
	mtx_View[3][1] =-dot(up, pos);
	mtx_View[0][2] =-front.x;
	mtx_View[1][2] =-front.y;
	mtx_View[2][2] =-front.z;
	mtx_View[3][2] = dot(front, pos);
	mtx_View[0][3] = 0.f;
	mtx_View[1][3] = 0.f; 
	mtx_View[2][3] = 0.f; 
	mtx_View[3][3] = 1.f;
}
void Camera::updateProjMtx()
{
	mtx_Proj = perspective(radians(fovy), aspect, z_near, z_far);
}
void Camera::setUniformTo(const Program& prg) const {
	prg.setUniform("cam_Pos", pos);
    prg.setUniform("mtx_Proj", mtx_Proj);
    prg.setUniform("mtx_View", mtx_View);
}
void Camera::copyFrom(const Camera& src) {
	fovy = src.fovy;
	z_near = src.z_near;
	pos = src.pos;
	pivot = src.pivot;
	global_up = src.global_up;
	mtx_View = src.mtx_View;
	mtx_Proj = src.mtx_Proj;
}