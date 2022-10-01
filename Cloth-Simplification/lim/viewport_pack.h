//
//	2022-10-01 / im dong ye 
//

#ifndef VIEWPORT_PACK_H
#define VIEWPORT_PACK_H
#include <vector>
#include <tuple>
#include "camera.h"
#include "model.h"
#include "scene.h"
#include "viewport.h"

namespace lim
{
	struct ViewportPack
	{
		Viewport* viewport=nullptr;
		Scene* scene=nullptr;
		Model* model=nullptr;
		Camera* camera=nullptr;
		const std::tuple<Viewport*, Scene*, Model*, Camera*>&& getTuple() const
		{
			return std::make_tuple(viewport, scene, model, camera);
		}
	};
	struct ViewportPackage
	{
		size_t size=0;
		std::vector<Viewport*> viewports;
		std::vector<Scene*> scenes;
		std::vector<Model*> models;
		std::vector<Camera*> cameras;
		std::vector<ViewportPack> data;
		void clear()
		{
			for( int i=0; i<size; i++ ) {
				delete cameras[i];
				delete models[i];
				delete scenes[i];
				delete viewports[i];
			}
			viewports.clear(); scenes.clear(); models.clear(); cameras.clear();
			data.clear();
		}
		void push_back(ViewportPack&& vp)
		{
			size++;
			viewports.push_back(vp.viewport);
			scenes.push_back(vp.scene);
			models.push_back(vp.model);
			cameras.push_back(vp.camera);
			data.push_back(vp);
		}
		void changeModel(int idx, Model* model)
		{
			scenes[idx]->setModel(model);
			models[idx]=model;
			data[idx].model=model;
		}
		const ViewportPack&& operator[](int idx) const
		{
			if( idx>=size ) throw std::out_of_range("viewport package");
			return std::move(data[idx]);
		}
		std::vector<ViewportPack>::iterator begin()
		{
			return std::begin(data);
		}
		std::vector<ViewportPack>::iterator end()
		{
			return std::end(data);
		}
	};
}
#endif VIEWPORT_PACK_H