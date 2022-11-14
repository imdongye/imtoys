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
	class HdrApp: public AppBase
	{
	private:
		const char const *exportPath = "result/";
	private:
		std::vector<Program*> programs;
		std::vector<Texture*> imgs;

	public:
		HdrApp(): AppBase(1280, 720, "imhdr")
		{
			Logger::get().log("       __      __      __\n");
			Logger::get().log("      /_/_ _  / /  ___/ /___\n");
			Logger::get().log("     / /  ' \/ _ \/ _  / __/\n");
			Logger::get().log("    /_/_/_/_/_//_/____/_/\n");;

			stbi_set_flip_vertically_on_load(true);

			programs.push_back(new Program("Normal Dot View"));
			programs.back()->attatch("pos_nor_uv.vs").attatch("fb_to_scr.fs").link();

			imgs.push_back(new Texture("images/memorial.jpg"));

			initCallback();
			imgui_modules::initImGui(window);
		}
		~HdrApp()
		{
			for( int i=0; i<programs.size(); i++ ) {
				delete programs[i];
			}


			imgui_modules::destroyImGui();
		}

	private:
		virtual void update() final
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			textureToBackBuf(imgs.back()->texID, imgs.back()->width, imgs.back()->height);

			renderImGui();
		}
		void renderImGui()
		{
			imgui_modules::beginImGui();

			ImGui::ShowDemoWindow();

			imgui_modules::endImGui(scr_width, scr_height);
		}

	private:
		void initCallback()
		{
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
		void keyCallback(int key, int scancode, int action, int mods)
		{

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
	};
}

#endif
