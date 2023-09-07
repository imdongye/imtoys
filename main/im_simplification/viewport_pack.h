//
//	2022-10-01 / im dong ye 
//

#ifndef __viewport_pack_
#define __viewport_pack_

#include <limbrary/model_view/scene.h>
#include <limbrary/model_view/viewport_with_camera.h>
#include <limbrary/model_view/auto_camera.h>
#include <stdexcept>

namespace lim
{
	struct ViewportPackage
	{
		size_t size=0;
		std::vector<Viewport*> viewports;
		std::vector<Scene*> scenes;
		std::vector<Model*> models;
		std::vector<AutoCamera*> cameras;
		void clear();
		void push_back(Viewport* vp, Scene* sc, Model* md, AutoCamera* cm);
		const std::tuple<Viewport*, Scene*, Model*, Camera*> operator[](int idx) const;
	};
}
#endif