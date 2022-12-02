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
//	멤버 변수 : snake_case
//	함수/멤버 함수 : camelCase
//	지역 변수, 파라미터 : camelCase
//	ps. 초기화용 param이 대문자가 없어서 구별 안될때 : _camelCase
// 
//	<C# 표준 위배 이유>
//	멤버함수에 파스칼 안쓰는 이유 : man.Dig() , 갑자기 대문자 나오는게 못생김
//	맴버변수에 '_'나 'm'를 붙여서 안쓰는 이유 : man->_age , ->_ 너무 못생김
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
// 
//	Todo:
//	1. 폴더구조
//	2. lim_max 등 tools로 헤더 나누기
//  3. 어느정도 완성되면 확장자 hpp로
//

#ifndef LIMCLUDE_H
#define LIMCLUDE_H

/* for vsprintf_s */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>

#include <glad/glad.h>
#define GLM_SWIZZLE // for var.zyx
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

/* JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC */
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
/* PNG, TGA, BMP */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define LIM_MAX(X, Y) ((X)>(Y))?(X):(Y)
#define LIM_MIN(X, Y) ((X)<(Y))?(X):(Y)
#define COMP_IMVEC2(X, Y) ((X).x==(Y).x)&&((X).y==(Y).y)

#include "imgui_modules.h"
#include "logger.h"
#include "app_pref.h"
#include "program.h"
#include "framebuffer.h"
#include "texture.h"
#include "tex_renderer.h"
#include "viewport.h"

#include "model_view/camera.h"
#include "model_view/light.h"
#include "model_view/mesh.h"
#include "model_view/model.h"
#include "model_view/model_loader.h"
#include "model_view/model_exporter.h"
#include "model_view/scene.h"

#include "application.h"

#endif 