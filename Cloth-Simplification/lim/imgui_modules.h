//
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. 한글 지원
//

#ifndef IMGUI_MODULES_H
#define IMGUI_MODULES_H

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

namespace lim
{
	namespace imgui_modules
	{
		static inline void initImGui(GLFWwindow* window)
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

			//float fontSize = 18.0f;// *2.0f;
			//io.Fonts->AddFontFromFileTTF("fonts/SpoqaHanSansNeo/SpoqaHanSansNeo-Bold.ttf", fontSize);
			//io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/SpoqaHanSansNeo/SpoqaHanSansNeo-Regular.ttf", fontSize);

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
		static inline void beginImGui()
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			//ImGuizmo::BeginFrame();
		}
		static inline void endImGui(float scr_width, float scr_height)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(scr_width, scr_height);

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
		static inline void destroyImGui()
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		// [SECTION] Example App: Docking, DockSpace / ShowExampleAppDockSpace()
		//-----------------------------------------------------------------------------
		// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
		// Note: You can use most Docking facilities without calling any API. You DO NOT need to call DockSpace() to use Docking!
		// - Drag from window title bar or their tab to dock/undock. Hold SHIFT to disable docking.
		// - Drag from window menu button (upper-left button) to undock an entire node (all windows).
		// - When io.ConfigDockingWithShift == true, you instead need to hold SHIFT to _enable_ docking/undocking.
		// About dockspaces:
		// - Use DockSpace() to create an explicit dock node _within_ an existing window.
		// - Use DockSpaceOverViewport() to create an explicit dock node covering the screen or a specific viewport.
		//   This is often used with ImGuiDockNodeFlags_PassthruCentralNode.
		// - Important: Dockspaces need to be submitted _before_ any window they can host. Submit it early in your frame! (*)
		// - Important: Dockspaces need to be kept alive if hidden, otherwise windows docked into it will be undocked.
		//   e.g. if you have multiple tabs with a dockspace inside each tab: submit the non-visible dockspaces with ImGuiDockNodeFlags_KeepAliveOnly.
		// (*) because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node,
		// because that window is submitted as part of the part of the NewFrame() call. An easy workaround is that you can create
		// your own implicit "Debug##2" window after calling DockSpace() and leave it in the window stack for anyone to use.
			// If you strip some features of, this demo is pretty much equivalent to calling DockSpaceOverViewport()!
		static inline void ShowExampleAppDockSpace()
		{
			// In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
			// In this specific demo, we are not using DockSpaceOverViewport() because:
			// - we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
			// - we allow the host window to have padding (when opt_padding == true)
			// - we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport() in your code!)
			// TL;DR; this demo is more complicated than what you would normally use.
			// If we removed all the options we are showcasing, this demo would become:
			//     void ShowExampleAppDockSpace()
			//     {
			//         ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
			//     }

			static bool dockspaceOpen = true;
			static bool opt_fullscreen = true;
			static bool opt_padding = false;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			if( opt_fullscreen )
			{
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}
			else
			{
				dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
			}

			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
			// and handle the pass-thru hole, so we ask Begin() to not render a background.
			if( dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode )
				window_flags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
			// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
			if( !opt_padding )
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
			if( !opt_padding )
				ImGui::PopStyleVar();

			if( opt_fullscreen )
				ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if( io.ConfigFlags & ImGuiConfigFlags_DockingEnable )
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}

			if( ImGui::BeginMenuBar() )
			{
				if( ImGui::BeginMenu("Options") )
				{
					// Disabling fullscreen would allow the window to be moved to the front of other windows,
					// which we can't undo at the moment without finer window depth/z control.
					ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
					ImGui::MenuItem("Padding", NULL, &opt_padding);
					ImGui::Separator();

					if( ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
					if( ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
					if( ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
					if( ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
					if( ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen) ) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
					ImGui::Separator();

					if( ImGui::MenuItem("Close", NULL, false, dockspaceOpen != NULL) )
						dockspaceOpen = false;
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::End();
		}

	}
}

#endif // !IMGUI_MODULES_H