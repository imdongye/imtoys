#include <limbrary/application.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/text.h>
#include <limbrary/tools/s_save_file.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/apps_selector.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/viewport.h>
#include <glad/glad.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
//#include <implot/implot.h>
#include <imguizmo/ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <algorithm>
#include <cstdlib>

#include <limbrary/using_in_cpp/std.h>
#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

namespace {
	string g_ini_file_path;
}



AppBase::AppBase(int winWidth, int winHeight, const char* title, bool vsync)
	:win_width(winWidth), win_height(winHeight)
{
	g_ptr = this;
	std::srand(time(0));
	log::reset();
	
	//
	//	GLFW
	//
	glfwSetErrorCallback([](int error, const char *description) {
		log::err("Glfw Error %d: %s\n", error, description);
		assert(true);
	});

	if( !glfwInit() ) {
		std::exit(-1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef WIN32
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#else //__APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	window = glfwCreateWindow(win_width, win_height, title, NULL, NULL);
	if( !window ) {
		log::err("Failed to create GLFW window\n");
		glfwTerminate();
		std::exit(-1);
	}

	// move to center
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
		monitor_max_fps = vidMode->refreshRate;
		glfwSetWindowPos(window, (vidMode->width-win_width)/2, (vidMode->height-win_height)/2);
		glfwShowWindow(window);
	}

	glfwGetFramebufferSize(window, &fb_width, &fb_height);
	aspect_ratio = fb_width/(float)fb_height;
	pixel_ratio =  fb_width/(float)win_width;

	glfwMakeContextCurrent(window);
	glfwSwapInterval((vsync)?1:0);





	//
	//	GLAD
	//
	if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
		log::err("Failed to initialize GLAD\n");
		std::exit(-1);
	}
	



	// OpenGL settings
	{
		// for hdr, ibl, post processing
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
	}




	//
	// print opengl status
	//
	{
		int iTemp, iTemp2;

		log::pure("Window size          : %d x %d\n", win_width, win_height);
		log::pure("GL Vendor            : %s\n", glGetString(GL_VENDOR));
		log::pure("GL Renderer          : %s\n", glGetString(GL_RENDERER));
		log::pure("GL Version (string)  : %s\n", glGetString(GL_VERSION));
		glGetIntegerv(GL_MAJOR_VERSION, &iTemp);
		glGetIntegerv(GL_MINOR_VERSION, &iTemp2);
		log::pure("Major, Minor         : %d.%d\n", iTemp, iTemp2);
		log::pure("GLSL Version         : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &iTemp);
		log::pure("#Vertex Attributes   : %d\n", iTemp);

#ifndef __APPLE__
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &iTemp);
		log::pure("#comp invocations    : %d\n", iTemp);
#endif
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &iTemp);
		log::pure("#Texture Slots       : %d\n", iTemp);
		glGetIntegerv(GL_MAX_SAMPLES, &iTemp);
		log::pure("#max ms samples      : %d\n", iTemp);
		log::pure("#monitor max fps     : %d\n", monitor_max_fps);
		
		log::pure("Current Path : %s\n\n", std::filesystem::current_path().string().c_str());
	}

	


	//
	// Register callback after glad initialization
	//
	{
		// lambda is better then std::bind
		// first call framebuffer resize, next window resize  when resize window
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* _, int width, int height) {
			g_ptr->is_size_changed = true;
			g_ptr->fb_width = width; g_ptr->fb_height = height;
			g_ptr->aspect_ratio = width/(float)height;
			for( auto& cb : g_ptr->framebuffer_size_callbacks ) cb(width, height);
		});
		glfwSetWindowSizeCallback(window, [](GLFWwindow* _, int width, int height) {
			g_ptr->win_width = width; g_ptr->win_height = height;
			g_ptr->pixel_ratio = g_ptr->fb_width/(float)g_ptr->win_width;
			for(auto& cb : g_ptr->win_size_callbacks ) cb(width, height);
		});
		glfwSetKeyCallback(window, [](GLFWwindow* _, int key, int scancode, int action, int mods) {
			for( auto& cb : g_ptr->key_callbacks ) cb(key, scancode, action, mods);
		});
		glfwSetMouseButtonCallback(window, [](GLFWwindow* _, int button, int action, int mods) {
			for( auto& cb : g_ptr->mouse_btn_callbacks ) cb(button, action, mods);
		});
		glfwSetScrollCallback(window, [](GLFWwindow* _, double xOff, double yOff) {
			vec2 scrollOff = vec2(float(xOff), float(yOff));
			for( auto& cb : g_ptr->scroll_callbacks) cb(scrollOff);
		});
		glfwSetCursorPosCallback(window, [](GLFWwindow* _, double xPos, double yPos) {
			vec2 curPos = vec2(float(xPos), float(yPos));
			for( auto& cb : g_ptr->cursor_pos_callbacks) cb(curPos);
		});
		glfwSetDropCallback(window, [](GLFWwindow* _, int count, const char **paths) {
			for( auto& cb : g_ptr->dnd_callbacks ) cb(count, paths);
		});
	}




	//
	// Setup Dear ImGui context
	//
	{ 	
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// custom app path for imgui.ini
		g_ini_file_path = fmtStrToBuf("%simgui.ini", g_app_dir);
		io.IniFilename = g_ini_file_path.c_str();
		
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		//float fontSize = 18.0f;// *2.0f;
		//io.Fonts->AddFontFromFileTTF("fonts/SpoqaHanSansNeo/SpoqaHanSansNeo-Bold.ttf", fontSize);
		//io.FontDefault = io.Fonts->AddFontFromFileTTF("fonts/SpoqaHanSansNeo/SpoqaHanSansNeo-Regular.ttf", fontSize);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifndef __APPLE__
		ImGui_ImplOpenGL3_Init("#version 460");
#else
		ImGui_ImplOpenGL3_Init("#version 410");
#endif
	}


	SaveFile::create();
	AssetLib::create();
	LimGui::resetEditors();
}





AppBase::~AppBase()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	//ImPlot::DestroyContext();
	ImGui::DestroyContext();

	SaveFile::destroy();
	AssetLib::destroy();

	glfwDestroyWindow(window);
	glfwTerminate();

	log::pure("\n\n\n");

	g_ptr = nullptr;
}





void AppBase::run()
{
	const ImGuiIO& io = ImGui::GetIO();

	while( !glfwWindowShouldClose(window) )
	{
		double nextTime =  glfwGetTime() + 1.0/custom_max_fps;

		delta_time = io.DeltaTime;

		glfwPollEvents(); // call callbacks

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		updateImGui();
		apps_selector::__drawGui();
		log::__drawViewerGui();
		
		ImGui::Render();

		is_focused = !io.WantCaptureMouse && !io.WantCaptureKeyboard;

		update();
		for( auto& cb : update_hooks ) {
			cb(delta_time);
		}

		// reset state
		is_size_changed = false;
		
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) 
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);

		if( custom_max_fps<1 ) {
			continue;
		}
		while( glfwGetTime() < nextTime ) {} // todo sleep
	}
}