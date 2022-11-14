//
//	2022-09-05 / im dong ye
// 
//	모든파일이 limclude만 include해야한다.
//	
//	헤더파일에서 include로 연결짓는게 의존성관리가 복잡해서 만들었음.
//	cocos2d, winapi 에서 사용했던 stdafx.h 와 비슷하게 만들었음
//	vs 설정에서 미리 컴파일된헤더로 설정할수있다는데 이경우에도 필요한지 모르겠음
//	에초에 이렇게 구조를 만든게 성능면에서 괜찮은건지 모르겠음
// 
//	namespace : snake case
//	멤버 변수 : snake case
//	지역 변수, param : camel case
//	함수/멤버 함수 : camel case
//	class/struct 이름 : pascal case
//	초기화용 param이 대문자가 없을때 : '_'+camel case
// 
//	
// 
//	Todo:
//	1. 폴더구조
//	2. lim_max 등 tools로 헤더 나누기
//

#ifndef LIMCLUDE_H
#define LIMCLUDE_H

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
#include <stb/stb_image_write.h>

#define LIM_MAX(X, Y) ((X)>(Y))?(X):(Y)
#define LIM_MIN(X, Y) ((X)<(Y))?(X):(Y)

#include "imgui_modules.h"
#include "logger.h"
#include "app_pref.h"
#include "program.h"
#include "texture.h"
#include "framebuffer.h"
#include "tex_renderer.h"
#include "application.h"
#include "app_hdr.h"

#endif 