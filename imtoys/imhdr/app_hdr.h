//
//  for test hdr image.
//	2022-11-14 / im dong ye
// 
//	ref: http://regex.info/blog/photo-tech/color-spaces-page2
//
//	TODO list:
//
//

#ifndef APP_HDR_H
#define APP_HDR_H

#include "color_awared_image.h"

namespace lim
{
	class AppHdr: public AppBase
	{
	public: 
		inline static constexpr const char *APP_DIR = "imhdr/";
        inline static constexpr const char *APP_NAME = "color aware image viewer";
		inline static constexpr const char *APP_DISC = "loader/display";
	private:
		const char const *exportPath = "result/";

		std::vector<Program*> programs;
		std::vector<ColorAwareImage*> imgs;
		std::vector<Viewport*> viewports;
	public:
		AppHdr(): AppBase(1950, 620, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			addImage("imhdr/images/italy-P3.jpg");
		}
		~AppHdr()
		{
			for( auto pg : programs ) delete pg;
			for( auto img : imgs ) delete img;
			for( auto vp : viewports ) delete vp;
		}
	private:
		void addImage(std::string_view path)
		{
			// color awared viewer
			imgs.push_back(new ColorAwareImage(path));
			imgs.back()->chromatic_adaptation = imgs.back()->profile.chromaticAdaptationTo(ICC::WHTPT_D65, 1);
			viewports.push_back(new Viewport(new Framebuffer(), imgs.back()->width, imgs.back()->height, Viewport::WM_FIXED_RATIO));
			viewports.back()->name = std::string(imgs.back()->name)+std::string(" - color awared");

			// direct viewer
			imgs.push_back(new ColorAwareImage(path));
			imgs.back()->profile.gamma = glm::vec3(1);
			imgs.back()->output_gamma = glm::vec3(1);
			imgs.back()->RGB2PCS = glm::mat3(1);
			imgs.back()->PCS2RGB = glm::mat3(1);
			imgs.back()->chromatic_adaptation = glm::mat3(1);

			viewports.push_back(new Viewport(new Framebuffer(), imgs.back()->width, imgs.back()->height, Viewport::WM_FIXED_RATIO));
			viewports.back()->name = std::string(imgs.back()->name)+std::string(" - direct view");
		}
	private:
		virtual void update() final
		{
			for( int i=0; i<imgs.size(); i++ ) {
				if( viewports[i]->window_opened == false ) {
					delete imgs[i]; delete viewports[i];
					imgs.erase(imgs.begin()+i);
					viewports.erase(viewports.begin()+i);
					i--;  continue;
				}
				imgs[i]->toFramebuffer(*viewports[i]->framebuffer);
				//texIDToFramebuffer(imgs[i]->tex_id, *viewports[i]->framebuffer, 1.f);
			}
		}
		virtual void renderImGui() final
		{
			imgui_modules::ShowExampleAppDockSpace([](){});
			ImGui::ShowDemoWindow();

			for( int i = viewports.size()-1; i>=0; i-- ) {
				viewports[i]->drawImGui();
			}

			ImGui::Begin("state##imhdr");
			//ImGui::Text("%d %d", viewports.back()->mousePos.x, viewports.back()->mousePos.y);
			for( int i=0; i<imgs.size(); i++ ) {
				Viewport& vp = *viewports[i];
				ColorAwareImage& img = *imgs[i];
				if( vp.focused ) {
					ImGui::Text(img.name);
					ImGui::Text(img.profile.name.c_str());
					break;
				}
			}

			ImGui::End();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
			//std::cout<<ImGui::GetFrameHeight();
		}
		virtual void cursorPosCallback(double xPos, double yPos) final
		{
			static double xOld, yOld, xOff, yOff=0;
			xOff = xPos - xOld;
			yOff = yOld - yPos;

			
			xOld = xPos;
			yOld = yPos;
		}
		virtual void mouseBtnCallback(int btn, int action, int mods) final
		{
		}
		virtual void dndCallback(int count, const char **paths) 
		{
			addImage(paths[0]);
		};
	};
}

#endif
