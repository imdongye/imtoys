#include "app_hdr.h"
#include <stb_image.h>
#include <glm/gtx/extended_min_max.hpp>
#include <imgui.h>

namespace
{
	
}

namespace lim
{
	AppHdr::AppHdr(): AppBase(1400, 780, APP_NAME)
	{
		

		addImage("assets/images/italy-P3.jpg");
	}
	AppHdr::~AppHdr()
	{
		for( auto pg : programs ) delete pg;
		for( auto img : imgs ) delete img;
		for( auto vp : viewports ) delete vp;
		imgs.clear();
	}
	void AppHdr::addImage(std::string_view path)
	{
		std::string vpName;
		Viewport* vp;	
		const int maxWidth = 800;
		int vpWidth, vpHeight;
		
		// color awared viewer
		imgs.push_back(new ColorAwareImage(path));
		vpWidth = glm::min(maxWidth, imgs.back()->width);
		vpHeight = (vpWidth==maxWidth)?maxWidth/imgs.back()->aspect_ratio : imgs.back()->height;

		vpName = std::string(imgs.back()->name)+std::string(" - color awared");
		vp = new Viewport(vpName, new FramebufferNoDepth());
		vp->resize(vpWidth, vpHeight);
		vp->setClearColor({1,1,1,1});
		vp->window_mode = Viewport::WM_FIXED_RATIO;
		viewports.push_back(vp);

		// direct viewer
		imgs.push_back(new ColorAwareImage(path));
		imgs.back()->profile.gamma = glm::vec3(1);
		imgs.back()->output_gamma = glm::vec3(1);
		imgs.back()->RGB2PCS = glm::mat3(1);
		imgs.back()->PCS2RGB = glm::mat3(1);
		imgs.back()->chromatic_adaptation = glm::mat3(1);

		vpName = std::string(imgs.back()->name)+std::string(" - direct view");
		vp = new Viewport(vpName, new FramebufferNoDepth());
		vp->resize(vpWidth, vpHeight);
		vp->window_mode = Viewport::WM_FIXED_RATIO;
		viewports.push_back(vp);
	}
	void AppHdr::update()
	{
		for( int i=0; i<imgs.size(); i++ ) {
			if( viewports[i]->is_opened == false ) {
				delete imgs[i]; delete viewports[i];
				imgs.erase(imgs.begin()+i);
				viewports.erase(viewports.begin()+i);
				i--;  continue;
			}
			imgs[i]->toFramebuffer(viewports[i]->getFb()); // todo
			//texIDToFramebuffer(imgs[i]->tex_id, *viewports[i]->getFb(), 1.f);
		}

		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	}
	void AppHdr::updateImGui()
	{
		//ImGui::DockSpaceOverViewport();

		for( int i = viewports.size()-1; i>=0; i-- ) {
			viewports[i]->drawImGui();
		}

		ImGui::Begin("state##imhdr");
		//ImGui::Text("%d %d", viewports.back()->mousePos.x, viewports.back()->mousePos.y);
		for( int i=0; i<imgs.size(); i++ ) {
			Viewport& vp = *viewports[i];
			ColorAwareImage& img = *imgs[i];
			if( vp.is_focused ) {
				ImGui::Text("%s", img.name.c_str());
				ImGui::Text("%s", img.profile.name.c_str());
				break;
			}
		}

		log::drawViewer("log##hdr");

		ImGui::End();
	}
	void AppHdr::dndCallback(int count, const char **paths) 
	{
		addImage(paths[0]);
	};
}
