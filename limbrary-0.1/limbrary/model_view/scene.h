//
//	2022-08-26 / im dong ye
//
//	ground모델을 직접가지고 다른 모델들은 참조한다.
//	참조모델들과 light로 viewport나 framebuffer에 렌더링한다.
// 
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. init destroy 생성자소멸자 사용
//  3. 여러 program 적용
//	4. shared pointer 사용
//	5. 여러 라이트
//

#ifndef __scene_h_
#define __scene_h_

#include "model.h"
#include "light.h"
#include <vector>


namespace lim
{
	struct Scene
	{
		std::vector<Model*> models;
		std::vector<Light*> lights;
	};
}
#endif
