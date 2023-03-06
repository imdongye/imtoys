//
//	2022-10-01 / im dong ye 
//

#ifndef VIEWPORT_PACK_H
#define VIEWPORT_PACK_H

namespace lim
{
	struct ViewportPackage
	{
		size_t size=0;
		std::vector<Viewport*> viewports;
		std::vector<Scene*> scenes;
		std::vector<Model*> models;
		std::vector<AutoCamera*> cameras;
		void clear()
		{
			for( int i=0; i<size; i++ ) {
				delete cameras[i];
				delete models[i];
				delete scenes[i];
				delete viewports[i];
			}
			viewports.clear(); scenes.clear(); models.clear(); cameras.clear();
		}
		void push_back(Viewport* vp, Scene* sc, Model* md, AutoCamera* cm)
		{
			size++;
			viewports.push_back(vp);
			scenes.push_back(sc);
			models.push_back(md);
			cameras.push_back(cm);
		}
		const std::tuple<Viewport*, Scene*, Model*, Camera*> operator[](int idx) const
		{
			if( idx>=size||idx<0 ) throw std::out_of_range("viewport package");
			return std::make_tuple(viewports[idx], scenes[idx], models[idx], cameras[idx]);
		}
	};
}
#endif VIEWPORT_PACK_H