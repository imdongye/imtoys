/*
	2023-07-14 / im dong ye

	Todo:
	convex hull nLogn
	natural pseudo(QR)로 구현
	Uniform B-spline
*/

#include "app_curve.h"
#include <limbrary/program.h>
#include <vector>
#include <limbrary/limgui.h>

using namespace std;
using namespace lim;
using namespace glm;

namespace {
	constexpr float GRID_STEP = 72.0f;
	constexpr int nr_types = 9;
	const char* const curve_type_strs[] = {
		"linear",
		"laglange",
		"bezier quadratic",
		"bezier cubic",
		"catmull rom, bessel",
		"bspline cubic",
		"cubic spline natural",
		"cubic spline closed",
		"cubic spline natural qr"
	};
	enum CurveType {
		LINEAR,
		LAGLANGE,
		BEZIER_QUADRATIC,
		BEZIER_CUBIC,
		CATMULL_ROM,
		BSPLINE_CUBIC,
		CUBIC_NATURAL,
		CUBIC_CLOSED,
		CUBIC_NATURAL_QR,
	};
	CurveType curve_type = LINEAR;


	bool is_closed = false;
	bool is_jacobi = false;
	int nr_iter = 5;

	constexpr int nr_pts = 10;
	vector<vec2> src_pts(nr_pts);
	constexpr int nr_segment_samples = 10; 
	constexpr float step_size = 1.f/nr_segment_samples;
	vector<vec2> draw_pts(nr_segment_samples*(nr_pts)+1);
}
/*
	Lagrange polynomial
	From: https://namu.wiki/w/%EB%9D%BC%EA%B7%B8%EB%9E%91%EC%A3%BC%20%EB%B3%B4%EA%B0%84%EB%B2%95
	#constraints : nr_pts=10
	#DOF : 10 => 9차식
	C_INF
	end point interpolation
	no inverse, use cardinal polynomial functions
	L_i(i) == 1
	f(t) = L_i(t)*p_i + ....
*/
static vec2 evalLaglange(int k, float t) {
	vec2 rst(0);
	float T = k + t;
	for(int i = 0; i < nr_pts; i++) {
		float L = 1;
		for(int j = 0; j < nr_pts; j++) {
			if (j != i) {
				L *= (T - j);
				L /= (i - j);
			}
		}
		rst += L * src_pts[i];
	}
	return rst;
}

/*
	From: https://namu.wiki/w/%EB%B2%A0%EC%A7%80%EC%97%90%20%EA%B3%A1%EC%84%A0
	de Casteljau step
	Convex : coef is not negative
	Affine Trasf invarient : transf samples == transf ctrl pts and sampling
*/
static vec2 evalBezierQuadratic(const vec2& p0, const vec2& p1, const vec2& p2, float t) {
	float invT1 = 1-t;
	float invT2 = invT1*invT1;
	return invT2*p0 + 2*invT1*t*p1 + t*t*p2;
}
static vec2 evalBezierCubic(const vec2& p0, const vec2& p1, const vec2& p2, const vec2& p3, float t) {
	float invT1 = 1-t;
	float invT2 = invT1*invT1;
	float invT3 = invT2*invT1;
	return invT3*p0 + 3*t*invT2*p1
		+ 3*t*t*invT1*p2 + t*t*t*p3;
}

/*
	From: https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline
	case of Hermite spline
		Piecewise
		C_1
		end point interpolation
		#constraints : 4 ( end points, tangents )
		#DOF : 4 => cubic
		but use bezier
	catmull rom
		tangents is side points direction
		not convex
		affine invarient
	bessel
		make tangent from 3 points
		to get tangent #DOF:3, #constraints:3
		use direct method
	if use bessel to all points (hermit spline with Bessel tangent)
		Overhauser spline
	
	...
*/
static vec2 evalCatmull(int k, float t) {
	glm::vec2 v0(0), v1(0);
	const float CM_DV = 2.f;
	if( k <= 0 ) // bessel
	{
		glm::mat3 A = { 0, 1, 4, 0, 1, 2, 1, 1, 1 };
		{
			glm::vec3 b = { src_pts[0].x, src_pts[1].x, src_pts[2].x };
			// 역행렬이 항상 존재함.
			glm::vec3 x = glm::inverse(A) * b;
			// x로 만들어진 2차식을 미분하면 2ax + b && x=0
			v0.x = x.y;
		}
		glm::vec3 b = { src_pts[0].y, src_pts[1].y, src_pts[2].y };
		glm::vec3 x = glm::inverse(A) * b;
		v0.y = x.y;

		v1 = (src_pts[k+2]-src_pts[k]) / CM_DV;
	}
	else if( k >= nr_pts-2 ) // bessel
	{
		glm::mat3 A = { (k-1)*(k-1), k*k, (k+1)*(k+1), (k-1), k, (k+1), 1, 1, 1 };
		{
			glm::vec3 b = { src_pts[k-1].x, src_pts[k].x, src_pts[k+1].x };
			glm::vec3 x = glm::inverse(A) * b;
			// ax^2 + bx + c (d/dx)=> 2ax + b (x=k+1)=> 2a(k+1)+b 
			v1.x = 2*x.x*(k+1) + x.y;
		}
		glm::vec3 b = { src_pts[k-1].y, src_pts[k].y, src_pts[k+1].y };
		glm::vec3 x = glm::inverse(A) * b;
		v1.y = 2*x.x*(k+1) + x.y;

		v0 = (src_pts[k+1]-src_pts[k-1]) / CM_DV;
	}
	else { // catmull
		v1 = (src_pts[k+2]-src_pts[k]) / CM_DV;
		v0 = (src_pts[k+1]-src_pts[k-1]) / CM_DV;
	}

	glm::vec2 c0 =  v0/3.f + src_pts[k];
	glm::vec2 c1 = -v1/3.f + src_pts[k+1];
	return evalBezierCubic(src_pts[k], c0, c1, src_pts[k+1], t);
}

/*
	From: https://en.wikipedia.org/wiki/B-spline
	basis spline

	make bezier ctrl pt from ctrl pt

	quadratic : C_1
	cubic : C_2 : second derivative matches
	not end point interpolation
	but convex
	affine invarient
*/
static vec2 evalBsplineCubic(int k, float t) {
	if( k<1 ) {
		k = 1;
		t = 0.f;
	}
	else if( k>nr_pts-3 ) {
		k = nr_pts-3;
		t = 1.f;
	}

	float B0 = 1.f/6.f * (-t*t*t + 3*t*t - 3*t + 1);
	float B1 = 1.f/6.f * ( 3*t*t*t - 6*t*t + 4);
	float B2 = 1.f/6.f * (-3*t*t*t + 3*t*t + 3*t + 1);
	float B3 = 1.f/6.f * t*t*t;

	return B0*src_pts[k-1] + B1*src_pts[k] + B2*src_pts[k+1] + B3*src_pts[k+2];
}

/*
	From: https://en.wikipedia.org/wiki/Spline_(mathematics)

	cubic spline with natural end condition
	strictly diagonally dominant => "iterative method"
	Ax = (D + L + U)x = b
	D*x = b - U*x - L*x
		4*x[i+1] = b[i] - 1*x[i+1] - 1*x[i-1] if i!=0 or i!=N-1
	x = inv(D)(b - U*x - L*x)
		x[i] = ( b[i] - 1*x[i+1] - 1*x[i-1] )/4.f

	jacobi iteration
		좌변의 x와 우변의 x를 각각두고 스왑한다.
	gauss seidel iteration
		좌변의 x와 우변의 x를 같게하여 iter 안에서 이전에 계산한것을 사용한다.
		더 빨리 수렴함.

	O(n^2*i) => 1000
	셈플마다 계산할필요없고 세그먼트에서 같이사용할수있어서 지금은 비효율적이다.
*/
static vec2 evalCubicNatural(int k, float t) {
	vector<vec2> D(nr_pts, vec2(0));

	// jacobi iteration
	if( is_jacobi )
	{
		// 여기서 D는 diagonal term이 아닌 x다. 식의 표기법
		vector<vec2> D0(nr_pts, vec2(0));
		vector<vec2> D1(nr_pts, vec2(0));
		for( int i=0; i<nr_iter; i++ ) {
			vector<vec2>& D_n0 = (i%2) ? D1 : D0;
			vector<vec2>& D_n1 = (i%2) ? D0 : D1;
			// 초기값
			D_n0[0]        = ( 3.f*(src_pts[1]-src_pts[0])               - D_n0[1]        ) / 2.f;
			D_n0[nr_pts-1] = ( 3.f*(src_pts[nr_pts-1]-src_pts[nr_pts-2]) - D_n0[nr_pts-2] ) / 2.f;

			for( int j=1; j<nr_pts-1; j++ ) {
				D_n1[j] = ( 3.f*(src_pts[j+1]-src_pts[j-1]) - D_n0[j+1] - D_n0[j-1] )/4.f;
			}
		}
		D = (nr_iter%2) ? D1 : D0;
	}

	// gauss seidel iteration
	else
	{
		for( int i=0; i<nr_iter; i++ ) {
			// 초기값
			D[0]        = ( 3.f*(src_pts[1]-src_pts[0])               - D[1]        ) / 2.f;
			D[nr_pts-1] = ( 3.f*(src_pts[nr_pts-1]-src_pts[nr_pts-2]) - D[nr_pts-2] ) / 2.f;
			for( int j=1; j < nr_pts-1; j++ ) {
				D[j] = ( 3.f*(src_pts[j+1]-src_pts[j-1]) - D[j+1] - D[j-1] )/4.f;
			}
		}
	}

	vec2 a = src_pts[k];
	vec2 b = vec2(D[k].x, D[k].y);
	vec2 c = 3.f*(src_pts[k+1] - src_pts[k]) - 2.f*D[k] - D[k+1];
	vec2 d = 2.f*(src_pts[k] - src_pts[k+1]) +     D[k] + D[k+1];
	return a + b*t + c*t*t + d*t*t*t;
}


static vec2 evalCubicClosed(int k, float t) {
	vector<vec2> D(nr_pts, vec2(0));

	// jacobi iteration
	if( is_jacobi )
	{
		// 여기서 D는 diagonal term이 아닌 x다. 식의 표기법
		vector<vec2> D0(nr_pts, vec2(0));
		vector<vec2> D1(nr_pts, vec2(0));
		for( int i=0; i<nr_iter; i++ ) {
			vector<vec2>& D_n0 = (i%2) ? D1 : D0;
			vector<vec2>& D_n1 = (i%2) ? D0 : D1;
			// 초기값
			D_n0[0]        = ( 3.f*(src_pts[1]-src_pts[0])               - D_n0[1] - D_n0[nr_pts-1] ) / 4.f;
			D_n0[nr_pts-1] = ( 3.f*(src_pts[nr_pts-1]-src_pts[nr_pts-2]) - D_n0[0] - D_n0[nr_pts-2] ) / 4.f;

			for( int j=1; j<nr_pts-1; j++ ) {
				D_n1[j] = ( 3.f*(src_pts[j+1]-src_pts[j-1]) - D_n0[j+1] - D_n0[j-1] )/4.f;
			}
		}
		D = (nr_iter%2) ? D1 : D0;
	}

	// gauss seidel iteration
	else
	{
		for( int i=0; i<nr_iter; i++ ) {
			// 초기값
			D[0]        = ( 3.f*(src_pts[1]-src_pts[0])               - D[1] - D[nr_pts-1] ) / 4.f;
			D[nr_pts-1] = ( 3.f*(src_pts[nr_pts-1]-src_pts[nr_pts-2]) - D[0] - D[nr_pts-2] ) / 4.f;
			for( int j=1; j < nr_pts-1; j++ ) {
				D[j] = ( 3.f*(src_pts[j+1]-src_pts[j-1]) - D[j+1] - D[j-1] )/4.f;
			}
		}
	}

	vec2 a = src_pts[k];
	vec2 b = vec2(D[k].x, D[k].y);
	vec2 c, d;
	if( k < nr_pts-1 ) {
		c = 3.f*(src_pts[k+1] - src_pts[k]) - 2.f*D[k] - D[k+1];
		d = 2.f*(src_pts[k] - src_pts[k+1]) +     D[k] + D[k+1];
	}
	else { // k == N - 1
		c = 3.f*(src_pts[0] - src_pts[k]) - 2.f*D[k] - D[0];
		d = 2.f*(src_pts[k] - src_pts[0]) +     D[k] + D[0];
	}
	return a + b*t + c*t*t + d*t*t*t;
}

static vec2 evalCubicNaturalQr(int k, float t) {
	//...

	return src_pts[k];
}

static vec2 evalLinear(int k, float t) {
	int nextIdx = (k+1)%nr_pts;
	return mix(src_pts[k], src_pts[nextIdx], t);
}

static vec2 evaluateCurve(int k, float t) {
	switch(curve_type) {
	case LAGLANGE:
		return evalLaglange(k, t);
	case BEZIER_QUADRATIC:
		if(k!=0)
			return src_pts[2];
		return evalBezierQuadratic(src_pts[0],src_pts[1],src_pts[2], t);
	case BEZIER_CUBIC:
		if(k!=0)
			return src_pts[3];
		return evalBezierCubic(src_pts[0],src_pts[1],src_pts[2],src_pts[3], t);
	case CATMULL_ROM:
		return evalCatmull(k, t);
	case BSPLINE_CUBIC:
		return evalBsplineCubic(k, t);
	case CUBIC_NATURAL:
		return evalCubicNatural(k, t);
	case CUBIC_CLOSED:
		return evalCubicClosed(k, t);
	case CUBIC_NATURAL_QR:
		return evalCubicNaturalQr(k, t);
	case LINEAR:
	default:
		return evalLinear(k,t);
	}
}

static void updateCurve() {
	draw_pts.clear();
	for( int i=0; i<nr_pts-1; i++ ) {
		for( float t=0; t<1.f; t+=step_size ) {
			draw_pts.push_back(evaluateCurve(i, t));
		}
	}
	if( is_closed ) {
		for( float t=0; t<1.f; t+=step_size ) {
			draw_pts.push_back(evaluateCurve(nr_pts-1, t));
		}
		draw_pts.push_back(evaluateCurve(nr_pts-1, 1.f));
	}
	else {
		draw_pts.push_back(evaluateCurve(nr_pts-2, 1.f));
	}
}

static void resetState() {
	for(int i=0; i<nr_pts; i++) {
		vec2& p = src_pts[i];
		p.x = i*GRID_STEP + GRID_STEP*2.f;
		p.y = GRID_STEP*3.f;
	}
	updateCurve();
}
AppCurve::AppCurve()
	: AppBase(1200, 480, APP_NAME, true)
{
	resetState();
}
AppCurve::~AppCurve()
{
}
void AppCurve::update()
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void AppCurve::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	log::drawViewer("logger##curve");

	static bool opt_enable_grid = true;
	static bool opt_enable_points = false;


	ImGui::Begin("controller##curve");
	if( ImGui::Combo("curve type", (int*)&curve_type, curve_type_strs, nr_types) ) {
		is_closed = (curve_type == CUBIC_CLOSED);
		updateCurve();
	}
	if( curve_type == CUBIC_NATURAL || curve_type == CUBIC_CLOSED ) {
		if( ImGui::Checkbox("is jacobi(or gauss seidel)", &is_jacobi) )
			updateCurve();
		if( ImGui::SliderInt("iterations", &nr_iter, 1, 100) )
			updateCurve();
	}
	ImGui::Checkbox("Enable grid", &opt_enable_grid);
	ImGui::Checkbox("Enable Points", &opt_enable_points);
	if( ImGui::Button("Reset") ) {
		resetState();
	}
	ImGui::TextWrapped("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");
	if( ImGui::CollapsingHeader("profiler") ) {
		LimGui::PlotVal("dt", "ms", delta_time);
		LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	}
	ImGui::End();


	ImGui::Begin("curve canvas");
	static ImVec2 scrolling(0.0f, 0.0f);
	ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

	// Draw border and background color
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
	draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	static bool is_picked = false;
	static int idx_hovered = -1;
	const bool is_hovered = ImGui::IsItemHovered(); // Hovered
	const bool is_active = ImGui::IsItemActive();   // Held
	const vec2 ori{canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y}; 
	const vec2 mouse_pos_in_canvas{io.MousePos.x-ori.x, io.MousePos.y-ori.y};


	// Add first and second point
	if( is_hovered && !is_picked && ImGui::IsMouseClicked(ImGuiMouseButton_Left) )
	{
		for( int i=0; i<nr_pts; i++ ) {
			vec2& p = src_pts[i];
			if( length2(p-mouse_pos_in_canvas) < 25.f ) {
				idx_hovered = i;
				break;
			}
		}
	}
	if( idx_hovered>=0 )
	{
		src_pts[idx_hovered] = mouse_pos_in_canvas;
		updateCurve();
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			idx_hovered = -1;
	}
	if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		scrolling.x += io.MouseDelta.x;
		scrolling.y += io.MouseDelta.y;
	}

	draw_list->PushClipRect(canvas_p0, canvas_p1, true);
	if( opt_enable_grid ) {
		for( float x = fmodf(scrolling.x, GRID_STEP); x<canvas_sz.x; x+=GRID_STEP ) {
			draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
		}
		for( float y = fmodf(scrolling.y, GRID_STEP); y<canvas_sz.y; y+=GRID_STEP ) {
			draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
		}
	}
	if( opt_enable_points ) {
		for( const vec2& p : draw_pts ) {
			draw_list->AddCircleFilled(toIG(ori+p), 2, IM_COL32(255, 255, 0, 255));
		}
	}
	else {
		const int nrPts = (int)draw_pts.size();
		for( int i = 0; i < nrPts-1; i++ ) {
			draw_list->AddLine(toIG(ori+draw_pts[i]), toIG(ori+draw_pts[i+1]), IM_COL32(255, 255, 0, 255), 2.0f);
		}
	}
	
	for( int i=0; i<nr_pts; i++ ) {
		const vec2& p = src_pts[i];
		if( i==idx_hovered ) {
			draw_list->AddCircleFilled(toIG(ori+p), 4, IM_COL32(255, 0, 0, 255));
		}
		else {
			draw_list->AddCircleFilled(toIG(ori+p), 4, IM_COL32(0, 0, 255, 255));
		}
	}
	draw_list->PopClipRect();

	ImGui::End();
}