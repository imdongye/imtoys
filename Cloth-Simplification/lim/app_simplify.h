//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. 헤더파일include 중복관리
//	4. figure out [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
//

#ifndef SIMPLYFY_APP_H
#define SIMPLYFY_APP_H

#include "application.h"
#include "../fqms.h"
//#include "simplify.h"

namespace lim
{
	class SimplifiyApp: public AppBase
	{
	private:
		Camera* mainCamera;
		std::vector<Scene*> scenes;
		std::vector<Viewport*> viewports;
		std::unordered_map<Viewport*, Scene*> connectedScene;

	public:
		SimplifiyApp()
		{
			scenes.push_back(new FirstScene());
			mainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -1), scr_width/(float)scr_height);
			viewports.push_back(new Viewport(mainCamera));
			connectedScene[viewports.back()] = scenes.back();

			init_callback();
			initImGui();
		}
		~SimplifiyApp()
		{
			for( Scene* scene : scenes ) delete scene;
			for( Viewport* viewport : viewports ) delete viewport;

			destroyImGui();
		}
	private:
		virtual void update() final
		{
			processInput();

			for( std::pair<Viewport*, Scene*> cs : connectedScene )
			{
				cs.second->update();
				cs.second->render(cs.first);
			}
			renderImGui();

			// render to screen
			//scenes.back()->render(0, scr_width, scr_height, mainCamera);
		}
	private:
		void initImGui()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
			//io.ConfigViewportsNoAutoMerge = true;
			//io.ConfigViewportsNoTaskBarIcon = true;

			float fontSize = 18.0f;// *2.0f;
			//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
			//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();

			// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
			ImGuiStyle& style = ImGui::GetStyle();
			if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 410");
		}
		void destroyImGui()
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
		void renderImGui()
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			//ImGuizmo::BeginFrame();
			{
				imgui_modules::ShowExampleAppDockSpace();

				ImGui::ShowDemoWindow();

				viewports.back()->renderImGui();

				{
					static float pct = 0.8f;
					static int counter = 0;

					ImGui::Begin("Simplify Options");                          // Create a window called "Hello, world!" and append into it.

					ImGui::Text("Hello, Worlds.");               // Display some text (you can use a format strings too)

					ImGui::SliderFloat("float", &pct, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

					// Buttons return true when clicked (most widgets return true when edited/activated)
					if( ImGui::Button("Simplify") )
					{
						fqms::simplifyModel(scenes[0]->models[1], pct);
						scenes[0]->models[1]->resetVRAM();
						counter++;
					}
					ImGui::SameLine();
					ImGui::Text("counter = %d", counter);

					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
					ImGui::End();
				}
			}
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2((float)scr_width, (float)scr_height);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Magic!
			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			// For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
		}


		void init_callback()
		{
			//wData.drop_callback = std::bind(&SimplifiyApp::drop_callback, this, std::placeholders::_1, std::placeholders::_2);
			wData.win_size_callback = [this](int width, int height) {
				return this->win_size_callback(width, height); };

			wData.key_callback = [this](int key, int scancode, int action, int mods) {
				return this->key_callback(key, scancode, action, mods); };

			wData.mouse_btn_callback = [this](int button, int action, int mods) {
				return this->mouse_btn_callback(button, action, mods); };

			wData.scroll_callback = [this](double xOffset, double yOffset) {
				return this->scroll_callback(xOffset, yOffset); };

			wData.cursor_pos_callback = [this](double xPos, double yPos) {
				return this->cursor_pos_callback(xPos, yPos); };

			wData.drop_callback = [this](int count, const char** path) {
				return this->drop_callback(count, path); };
		}
		void processInput()
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS )
				{
					float moveSpeed = 1.6f;
					if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS )
						moveSpeed = 3.2f;
					if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::FORWARD, deltaTime, moveSpeed);
					if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::BACKWARD, deltaTime, moveSpeed);
					if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::LEFT, deltaTime, moveSpeed);
					if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::RIGHT, deltaTime, moveSpeed);
					if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::UP, deltaTime, moveSpeed);
					if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::DOWN, deltaTime, moveSpeed);
				}
			}
		}
		void key_callback(int key, int scancode, int action, int mode)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
				   && key == GLFW_KEY_Z )
				{
					if( action == GLFW_PRESS )
					{
						camera.readyPivot();
						camera.updatePivotViewMat();
					}
					else if( action == GLFW_RELEASE )
					{
						camera.readyFree();
						camera.updateFreeViewMat();
					}
				}
			}
			if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose(window, GL_TRUE);
		}
		void cursor_pos_callback(double xPos, double yPos)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				const float w = scr_width;
				const float h = scr_height;

				float xoff = xPos - vp->cursor_pos_x;
				float yoff = vp->cursor_pos_y - yPos; // inverse

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) )
				{
					if( glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS )
					{
						if( glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS )
						{
							camera.shiftDist(xoff/w*160.f);
							camera.shiftZoom(yoff/h*160.f);
						}
						else
						{
							camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						}
						camera.updatePivotViewMat();
					}
					else
					{
						camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						camera.updateFreeViewMat();
					}

				}
				vp->cursor_pos_x = xPos; vp->cursor_pos_y = yPos;
			}
		}
		void mouse_btn_callback(int button, int action, int mods)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->hovered == false ) continue;
				Camera& camera = *vp->camera;
				if( action == GLFW_PRESS )
				{
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					vp->cursor_pos_x = xpos;
					vp->cursor_pos_y = ypos;
				}
			}
		}
		void scroll_callback(double xoff, double yoff)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				camera.shiftZoom(yoff*10.f);
				camera.updateProjMat();
			}
		}
		void win_size_callback(int width, int height)
		{
			scr_width = width;
			scr_height = height;
		}
		void drop_callback(int count, const char** paths)
		{
			FirstScene* fs = dynamic_cast<FirstScene*>(scenes[0]);
			for( int i = 0; i < count; i++ )
			{
				// 여러개들어왔을때 모델파일만 골라서 입력받게 조건처리
				fs->loadModel(paths[i]);
			}

			/*
			for( Viewport* vp : viewports )
			{
				// 다른 window에서 드래그할때 hovered 인식안됨
				if( vp->focused == false ) continue;

				FirstScene* fs = dynamic_cast<FirstScene*>(connectedScene[vp]);
				if( fs==nullptr )
				{
					fprintf(stderr, "\nerror : scene down casting error\n");
					break;
				}

				for( int i = 0; i < count; i++ )
				{
					fs->loadModel(paths[i]);
				}
			}*/
		}
	};

}

#endif
