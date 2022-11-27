//
//  for test hdr image.
//	2022-11-14 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_HDR_H
#define APP_HDR_H

namespace lim
{
	class AppHdr: public AppBase
	{
	public: 
		inline static constexpr const char *APP_NAME = "imhdr";
		inline static constexpr const char *APP_DISC = "color aware image viewer";
	private:
		const char const *exportPath = "result/";

		std::vector<Program*> programs;
		std::vector<Texture*> imgs;
		std::vector<Viewport*> viewports;
	public:
		AppHdr(): AppBase(1280, 720, "imhdr")
		{
			stbi_set_flip_vertically_on_load(true);

			programs.push_back(new Program("Normal Dot View"));
			//programs.back()->attatch("tex_to_quad.vs").attatch("tex_to_quad.fs").link();

			imgs.push_back(new Texture("images/memorial.jpg", GL_SRGB8));
			viewports.push_back(new Viewport(new Framebuffer(), imgs.back()->width, imgs.back()->height, true));
		}
		~AppHdr()
		{
			for( auto pg : programs ) delete pg;
			for( auto img : imgs ) delete img;
			for( auto vp : viewports ) delete vp;
		}

	private:
		virtual void update() final
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			//textureToFBO(imgs.back()->texID, 500, 500);
			//textureToFBO(imgs.back()->texID, imgs.back()->width, imgs.back()->height, viewports.back()->framebuffer->fbo);
			textureToFramebuffer(imgs.back()->texID, viewports.back()->framebuffer);
		}
		virtual void renderImGui() final
		{
			imgui_modules::ShowExampleAppDockSpace([](){});

			ImGui::ShowDemoWindow();

			for( int i = viewports.size()-1; i >= 0; i-- ) {
				viewports[i]->drawImGui();
			}

			ImGui::Begin("state");
			ImGui::Text("%d %d", viewports.back()->mousePos.x, viewports.back()->mousePos.y);
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
			printf("a");
		}
	};
}

#endif
