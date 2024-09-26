#include "app_hdr.h"
#include <stb_image.h>
#include <glm/gtx/extended_min_max.hpp>
#include <limbrary/tools/text.h>
#include <limbrary/tools/limgui.h>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

namespace
{
	constexpr int max_initial_img_width = 800;
}


AppHdr::AppHdr(): AppBase(1400, 780, APP_NAME)
{
	addImage("assets/images/italy-P3.jpg");
	dnd_callbacks[this] = [this](int count, const char **paths) {
		addImage(paths[0]);
	};
}
AppHdr::~AppHdr()
{
	for( auto pg : programs ) delete pg;
	for( auto img : imgs ) delete img;
	for( auto vp : viewports ) delete vp;
	imgs.clear();
}
void AppHdr::addImage(const char* path)
{	
	ColorAwareImage* img;
	Viewport* vp;
	ivec2 vpSize;
	const char* vpName;

	// color awared viewer
	img = new ColorAwareImage(path);
	imgs.push_back(img);
	if( max_initial_img_width < img->size.x ) {
		vpSize.x = max_initial_img_width;
		vpSize.y = max_initial_img_width / img->aspect_ratio;
	} else {
		vpSize = img->size;
	}

	vpName = fmtStrToBuf("%s - color awared", imgs.back()->name.c_str());
	vp = new Viewport(new FramebufferNoDepth(), vpName);
	viewports.push_back(vp);
	vp->window_mode = Viewport::WM_FIXED_RATIO;
	vp->resize(vpSize);
	vp->getFb().clear_color = {1,1,1,1};

	// direct viewer
	imgs.push_back(new ColorAwareImage(path));
	imgs.back()->profile.gamma = glm::vec3(1);
	imgs.back()->output_gamma = glm::vec3(1);
	imgs.back()->RGB2PCS = glm::mat3(1);
	imgs.back()->PCS2RGB = glm::mat3(1);
	imgs.back()->chromatic_adaptation = glm::mat3(1);

	vpName = fmtStrToBuf("%s - direct view", imgs.back()->name.c_str());
	vp = new Viewport(new FramebufferNoDepth(), vpName);
	viewports.push_back(vp);
	vp->window_mode = Viewport::WM_FIXED_RATIO;
	vp->resize(vpSize);
}
void AppHdr::update()
{
	// clear backbuffer
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
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
}
void AppHdr::updateImGui()
{
	//ImGui::DockSpaceOverViewport();

	for( int i = (int)viewports.size()-1; i>=0; i-- ) {
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

	ImGui::End();
}