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
	private:
		const char const *exportPath = "result/";
	private:
		std::vector<Program*> programs;
		std::vector<Texture*> imgs;
		std::vector<Viewport*> viewports;
	public:
		AppHdr(): AppBase(1280, 720, "imhdr")
		{
			stbi_set_flip_vertically_on_load(true);
			initCallback();
			imgui_modules::initImGui(window);

			programs.push_back(new Program("Normal Dot View"));
			programs.back()->attatch("tex_to_quad.vs").attatch("tex_to_quad.fs").link();

			imgs.push_back(new Texture("images/memorial.jpg", GL_SRGB8));
			viewports.push_back(new Viewport(new Framebuffer(), imgs.back()->width, imgs.back()->height, true));
		}
		~AppHdr()
		{
			imgui_modules::destroyImGui();
		}

	private:
		virtual void update() override final
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			//textureToFBO(imgs.back()->texID, 500, 500);
			//textureToFBO(imgs.back()->texID, imgs.back()->width, imgs.back()->height, viewports.back()->framebuffer->fbo);
			textureToFramebuffer(imgs.back()->texID, viewports.back()->framebuffer);

			renderImGui();
		}
		void renderImGui()
		{
			imgui_modules::beginImGui();

			imgui_modules::ShowExampleAppDockSpace([](){});

			ImGui::ShowDemoWindow();

			for( int i = viewports.size()-1; i >= 0; i-- ) {
				viewports[i]->drawImGui();
			}

			ImGui::Begin("state");
			ImGui::Text("%d %d", viewports.back()->mousePos.x, viewports.back()->mousePos.y);
			ImGui::End();

			imgui_modules::endImGui(scr_width, scr_height);
		}

	private:
		void keyCallback(int key, int scancode, int action, int mods)
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		void cursorPosCallback(double xPos, double yPos)
		{
			static double xOld, yOld, xOff, yOff=0;
			xOff = xPos - xOld;
			yOff = yOld - yPos;

			
			xOld = xPos;
			yOld = yPos;
		}
		void mouseBtnCallback(int button, int action, int mods)
		{
		}
		void scrollCallback(double xOff, double yOff)
		{
			
		}
		void dndCallback(int count, const char **paths)
		{
			
		}
		void initCallback()
		{
			/* lambda is better then std::bind */
			w_data.key_callbacks.push_back([this](int key, int scancode, int action, int mods)
			{ return this->keyCallback(key, scancode, action, mods); });
			w_data.mouse_btn_callbacks.push_back([this](int button, int action, int mods)
			{ return this->mouseBtnCallback(button, action, mods); });
			w_data.scroll_callbacks.push_back([this](double xOff, double yOff)
			{ return this->scrollCallback(xOff, yOff); });
			w_data.cursor_pos_callbacks.push_back([this](double xPos, double yPos)
			{ return this->cursorPosCallback(xPos, yPos); });
			w_data.dnd_callbacks.push_back([this](int count, const char **path)
			{ return this->dndCallback(count, path); });
		}
	};
}

#endif
