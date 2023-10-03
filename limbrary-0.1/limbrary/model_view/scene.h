/*

2022-08-26 / im dong ye

Note:
renderer에서 사용하기위한 단순 컨테이너

*/

#ifndef __scene_h_
#define __scene_h_

#include "model.h"
#include "light.h"
#include <vector>


namespace lim
{
	// Copyable
	struct Scene
	{
		std::vector<const Model*> models;
		std::vector<const Light*> lights;
	};
}
#endif
