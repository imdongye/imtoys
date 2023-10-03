#include <limbrary/application.h>
#include <limbrary/log.h>
#include <limbrary/app_pref.h>
#include <limbrary/asset_lib.h>
#include <limbrary/viewport.h>
#include <glad/glad.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <backend/imgui_impl_glfw.h>
#include <backend/imgui_impl_opengl3.h>


namespace lim
{
	AppBase::AppBase(int winWidth, int winHeight, const char* title)
		:win_width(winWidth), win_height(winHeight)
	{
		AppPref::get().app = this;
		glfwSetErrorCallback([](int error, const char *description) {
			log::err("Glfw Error %d: %s\n", error, description);
		});

		if( !glfwInit() ) std::exit(-1);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
		// (1) invisible setting before creating for move center
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		window = glfwCreateWindow(win_width, win_height, title, NULL, NULL);

		if( window == NULL ) {
			log::err("Failed to create GLFW window\n");
			glfwTerminate();
			std::exit(-1);
		}

		/* window setting */
		applyWindowSetting();

		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		aspect_ratio = fb_width/(float)fb_height;
		pixel_ratio =  fb_width/(float)win_width;


		glfwMakeContextCurrent(window);
		// default is 0 then make tearing so must 1 to buffer swap when all buffer is showen
		// 수직동기화 : 1
		glfwSwapInterval(0); // vsync

		if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
			log::err("Failed to initialize GLAD\n");
			std::exit(-1);
		}

		// register callback after glad initialization
		initGlfwCallbacks();

		printVersionAndStatus();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
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
		ImGui_ImplOpenGL3_Init("#version 410");

		AssetLib::create();
	}

	AppBase::~AppBase()
	{
		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		AppPref::get().saveToFile();
		AssetLib::destroy();

		glfwDestroyWindow(window);
		glfwTerminate();

		log::pure("\n\n\n");
		log::clear();
	}

	void AppBase::run()
	{
		ImGuiIO& io = ImGui::GetIO();
		double lastTime=glfwGetTime();

		while( !glfwWindowShouldClose(window) ) {
			double currentTime = glfwGetTime();
			delta_time = (float)(currentTime - lastTime);
			lastTime = currentTime;

			io.DisplaySize = ImVec2(fb_width, fb_height); // todo?
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			//ImGuizmo::BeginFrame();
			renderImGui();
			_draw_appselector();
			ImGui::Render();

			update();
			for( auto& cb : update_hooks ) 
				cb(delta_time);
			
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwPollEvents();
			glfwSwapBuffers(window);
		}
	}

	void AppBase::initGlfwCallbacks()
	{
		glfwSetWindowUserPointer(window, this);

		/* lambda is better then std::bind */

		// first call framebuffer resize, next window resize  when resize window
		framebuffer_size_callbacks[this] = [this](int w, int h) {
			framebufferSizeCallback(w, h);
		};
		key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
			keyCallback(key, scancode, action, mods); 
		};
		mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
			mouseBtnCallback(button, action, mods); 
		};
		scroll_callbacks[this] = [this](double xOff, double yOff) {
			scrollCallback(xOff, yOff); 
		};
		cursor_pos_callbacks[this] = [this](double x, double y) {
			mouse_pos = {x,y};
			cursorPosCallback(x, y); 
		};
		dnd_callbacks[this] = [this](int count, const char **path) {
			dndCallback(count, path); 
		};

		glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
			AppBase& app = *AppPref::get().app;
			app.win_width = width; app.win_height = height;
			app.aspect_ratio = width/(float)height;
			app.pixel_ratio = app.fb_width/(float)app.win_width;
			for(auto& cb : app.win_size_callbacks ) cb(width, height);
		});
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
			AppBase& app = *AppPref::get().app;
			app.fb_width = width; app.fb_height = height;
			for( auto& cb : app.framebuffer_size_callbacks ) cb(width, height);
		});
		glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
			AppBase& app = *AppPref::get().app;
			for( auto& cb : app.key_callbacks ) cb(key, scancode, action, mods);
		});
		glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
			AppBase& app = *AppPref::get().app;
			for( auto& cb : app.mouse_btn_callbacks ) cb(button, action, mods);
		});
		glfwSetScrollCallback(window, [](GLFWwindow *window, double xOff, double yOff) {
			AppBase& app = *AppPref::get().app;
			for( auto& cb : app.scroll_callbacks) cb(xOff, yOff);
		});
		glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xPos, double yPos) {
			AppBase& app = *AppPref::get().app;
			for( auto& cb : app.cursor_pos_callbacks) cb(xPos, yPos);
		});
		glfwSetDropCallback(window, [](GLFWwindow *window, int count, const char **paths) {
			AppBase& app = *AppPref::get().app;
			for( auto& cb : app.dnd_callbacks ) cb(count, paths);
		});
	}

	void AppBase::applyWindowSetting()
	{
		// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		// glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		// (해상도, 채널, 주사율)
		const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);

		if(is_windowed)
		{
			if (is_borderless)
			{
				glfwSetWindowMonitor(window, nullptr, 0, 0,
						vidMode->width, vidMode->height, vidMode->refreshRate);
			}
			else
			{
				glfwSetWindowPos(window, (vidMode->width-win_width)/2, (vidMode->height-win_height)/2);
			}
		}
		else
		{
			glfwSetWindowMonitor(window, monitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
		}

		
		glfwShowWindow(window);
	}

	void AppBase::printVersionAndStatus()
	{
		const GLubyte *renderer = glGetString(GL_RENDERER);
		const GLubyte *vendor = glGetString(GL_VENDOR);
		const GLubyte *version = glGetString(GL_VERSION);
		const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

		GLint major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		log::pure("Frame size           : %d x %d\n", win_width, win_height);
		log::pure("GL Vendor            : %s\n", vendor);
		log::pure("GL Renderer          : %s\n", renderer);
		log::pure("GL Version (string)  : %s\n", version);
		log::pure("GLSL Version         : %s\n", glslVersion);
		int nrAttributes, nrTextureUnits;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
		log::pure("Maximum nr of vertex attributes supported: %d\n", nrAttributes);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &nrTextureUnits);
		log::pure("Maximum nr of texture slots supported: %d\n", nrTextureUnits);
		log::pure("Current path is %s\n\n", std::filesystem::current_path().string().c_str());
	}
}