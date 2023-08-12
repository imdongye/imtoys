//
//	2022-09-05 / im dong ye
//
//	모든파일이 limclude만 include해야한다.
//
//	헤더파일에서 include로 연결짓는게 의존성관리가 복잡해서 만들었음.
//	cocos2d, winapi 에서 사용했던 stdafx.h 와 비슷하게 만들었음
//	vs 설정에서 미리 컴파일된헤더로 설정할수있다는데 이경우에도 필요한지 모르겠음
//
//	circular dependency(include) 는 최대한 피하되 어쩔수없다면 구현부를 ipp파일로 나눠서
//	꼬인헤더가 모두 선언된 시점에 구현부를 include한다.
//	EX) booster(asio)라이브러리의 hpp와 impl/*.ipp
//
//	[ 이름 규칙 ]
//	namespace : snake_case
//	class/struct 이름 : PascalCase
//	멤버 상수 : UPPER_SNAKE_CASE
//	멤버 변수, 전역 변수 : snake_case
//	함수/멤버 함수 : camelCase
//	지역 변수, 파라미터 : camelCase
//	ps. 초기화용 param이 대문자가 없어서 구별 안될때 : _param
//		-> 앞에 붙는 _ 는 임시의 의미
//
//	<C# 표준 위배 이유>
//	멤버함수에 파스칼 안쓰는 이유 : man.Dig() , 갑자기 대문자 나오는게 못생김
//	맴버변수에 '_'나 'm'를 붙여서 안쓰는 이유 : 함수와 변수는 동사()와 명사로 구분이 확실하고,
//											   멤버변수를 접근하면 cat.mAge 로 이미 멤버 변수임을 알고있는데
//											   m을 통해 추가로 구분해야할 필요가 없어보이고 못생겨짐
//  함수의 매개변수에서 Class이름등으로 어떤객체인지 확실할때는 약어로 쓴다.
//	ex) void init(Player p, int _age)
//
//	<헤더 온리 단점>
//	멤버함수 구현에 필요한 함수를 감추지 못함. 구현부 cpp에선 글로벌 함수로 막 써도됨
//	circular dependency(include)가 없는게 좋긴 하지만 필요한경우 살짝 복잡해짐
//	<헤더 온리 장점>
//	선언부 수정시 한번에 가능
//	구현에 필요한 멤버변수 같은파일에서 확인가능
//	cpp마다 생성되는 obj파일이 하나라 프로그램 크기가 작을것이다?
//
//	member initialization list는 객체에 대해서만 쓴다. 길어지면 가독성 안좋음
//	나머지는 생성자안에서 정의
//
//	int *a, *b; int *c;
//	int& a = b;
//	포인터는 변수명앞에 레퍼런스는 자료형 뒤에 붙인다.
//	레퍼런스는 선언과동시에 초기화해야되므로 연속선언이 필요없고 가독성이 더 좋기때문에
//
//	glEnable/Disable은 사용후 초기값으로 복구 시켜두기
//
//
//	Todo:
//  1. 어느정도 완성되면 확장자 hpp로
//	2. premake로 멀티플렛폼 실행환경 지원
//	3. imgui, glad, nanovg lib로 만들었는데 왜 컴파일타임 0.3초밖에 안줄었지 어디서 느려지는거지
//	4. nanovg와 imgui stb 의존성중복 해결
// 
// 
// https://stackoverflow.com/questions/67677799/problem-when-using-assimp-library-together-with-stb-image
//

#ifndef LIMCLUDE_H
#define LIMCLUDE_H

// #pragma comment(lib, "OpenGL32.lib")
// #pragma comment(lib, "lib/glfw3.lib")

/* for vsprintf_s */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
// for min max dup define error when include windows.h
#define NOMINMAX

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>
#include <map>

#include <glad/glad.h>
// #define GLM_SWIZZLE // for var.zyx
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
// #include <glm/gtx/norm.hpp>
#include <GLFW/glfw3.h>

/* JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC */
//#define STB_IMAGE_IMPLEMENTATION //in nanovg.lib
#include <stb/stb_image.h>
/* PNG, TGA, BMP */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
// #define STB_TRUETYPE_IMPLEMENTATION in nanovg.lib
#include <stb/stb_truetype.h>

#include "declaration.h"

#include "logger.h"
#include "asset_lib.h"
#include "imgui_modules.h"
#include "app_pref.h"
#include "program.h"
#include "framebuffer.h"
#include "texture.h"
#include "viewport.h"

#include "model_view/camera.h"
#include "model_view/light.h"
#include "model_view/mesh.h"
#include "model_view/mesh_generator.h"
#include "model_view/model.h"
#include "model_view/model_loader.h"
#include "model_view/model_exporter.h"
#include "model_view/scene.h"

#include "asset_lib.inl"

#include "application.h"
#include "model_view/auto_camera.h"

//#include <nanovg/nanovg.h>
//#define NANOVG_GL3_IMPLEMENTATION
//#include <nanovg/nanovg_gl.h>

#endif
