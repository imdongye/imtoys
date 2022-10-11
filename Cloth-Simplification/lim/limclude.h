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
//	model과 light가 서로 의존성이 있는데 선언부와 구현부로 안나누고 header only로 구현할방법이 있나?
// 
//	Todo:
//	1. 폴더구조
//	2. lim_max 등 tools로 헤더 나누기
//

#ifndef LIMCLUDE_H
#define LIMCLUDE_H

#include <glad/glad.h>
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>

#define LIM_MAX(X, Y) ((X)>(Y))?(X):(Y)
#define LIM_MIN(X, Y) ((X)<(Y))?(X):(Y)

#include "logger.h"
#include "app_pref.h"
#include "program.h"
#include "camera.h"
#include "texture.h"
#include "imgui_modules.h"
#include "framebuffer.h"
#include "light.h"
#include "mesh.h"
#include "model.h"
#include "model_loader.h"
#include "model_exporter.h"
#include "viewport.h"
#include "scene.h"
#include "viewport_pack.h"
#include "application.h"
#include "../fqms.h"
#include "app_simplify.h"

#endif 