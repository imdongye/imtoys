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
//	Todo:
//	1.폴더구조
//

#pragma once

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>
#include <functional>
#include <vector>

#include "program.h"
#include "camera.h"
#include "texture.h"
#include "imgui_modules.h"
#include "framebuffer.h"
#include "light.h"
#include "mesh.h"
#include "model.h"
#include "viewport.h"
#include "scene.h"
#include "application.h"
#include "../fqms.h"
#include "app_simplify.h"