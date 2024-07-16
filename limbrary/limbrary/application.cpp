#include <limbrary/application.h>
#include <limbrary/log.h>
#include <limbrary/app_prefs.h>
#include <limbrary/asset_lib.h>
#include <limbrary/viewport.h>
#include <glad/glad.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
//#include <implot/implot.h>
#include <imguizmo/ImGuizmo.h>
#include <backend/imgui_impl_glfw.h>
#include <backend/imgui_impl_opengl3.h>
#include <algorithm>

using namespace lim;

namespace
{
	double _refresh_time = 0.0;
	glm::vec2 _fps_graph_data[110] = {};

	void _drawProfilerFps() {
		static bool isFpsOpened = false;
		if( ImGui::IsKeyPressed(ImGuiKey_F2, false) )
			isFpsOpened = !isFpsOpened;
		if( isFpsOpened ) {
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
			const float PAD = 10.0f;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 workPos = viewport->WorkPos;
			ImVec2 windowPos = {workPos.x+PAD, workPos.y+PAD+PAD*1.8f};

			
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
			ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
			if( ImGui::Begin("debug overlay", &isFpsOpened, window_flags) )
			{
				static int fpsGraphOffset = 0;
				const int refreshRate = lim::AssetLib::get().app->refresh_rate;
				const float framerate = ImGui::GetIO().Framerate;
				float time = ImGui::GetTime();
				if( _refresh_time == 0.0 )
					_refresh_time = time;
				while( _refresh_time < time )
				{
					_fps_graph_data[fpsGraphOffset] = {time, refreshRate};
					fpsGraphOffset = (fpsGraphOffset + 1) % IM_ARRAYSIZE(_fps_graph_data);
					_refresh_time += 1.0f / (float)refreshRate;
				}

				const int mid = refreshRate;
				const int pad = mid/4;
				const char* fpsStr = lim::fmtStrToBuf("%.3f ms/frame, %.2fFPS", 1000.0f/framerate, framerate);
				ImGui::PlotHistogram("##FPS", &_fps_graph_data[0].y, IM_ARRAYSIZE(_fps_graph_data), fpsGraphOffset, fpsStr, mid-pad, mid+pad, ImVec2(0, 40.0f), 8);

				
			}
			ImGui::PopStyleVar();
			ImGui::End();
		}
	}
	void _resetProfiler()
	{
		_refresh_time = 0.0;
		std::fill_n(_fps_graph_data, IM_ARRAYSIZE(_fps_graph_data), glm::vec2(0));
	}
}





AppBase::AppBase(int winWidth, int winHeight, const char* title, bool vsync)
	:win_width(winWidth), win_height(winHeight)
{
	//
	//	GLFW
	//
	glfwSetErrorCallback([](int error, const char *description) {
		log::err("Glfw Error %d: %s\n", error, description);
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
	//glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
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
		refresh_rate = vidMode->refreshRate;
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
	



	//
	// print opengl status
	//
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
		int iTemp;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &iTemp);
		log::pure("#Vertex Attributes   : %d\n", iTemp);

		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &iTemp);
		log::pure("#comp invocations    : %d\n", iTemp);
		
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &iTemp);
		log::pure("#Texture Slots       : %d\n", iTemp);
		log::pure("Current Path : %s\n\n", std::filesystem::current_path().string().c_str());
	}

	

	// init singleton
	glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
	srand(time(0));
	AppPrefs::create();
	AssetLib::create(this);


	//
	// Register callback after glad initialization
	//
	{
		//glfwSetWindowUserPointer(window, this);

		// lambda is better then std::bind
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
			AppBase& app = *AssetLib::get().app;
			app.win_width = width; app.win_height = height;
			app.aspect_ratio = width/(float)height;
			app.pixel_ratio = app.fb_width/(float)app.win_width;
			for(auto& cb : app.win_size_callbacks ) cb(width, height);
		});
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
			AppBase& app = *AssetLib::get().app;
			app.fb_width = width; app.fb_height = height;
			for( auto& cb : app.framebuffer_size_callbacks ) cb(width, height);
		});
		glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
			for( auto& cb : AssetLib::get().app->key_callbacks ) cb(key, scancode, action, mods);
		});
		glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
			for( auto& cb : AssetLib::get().app->mouse_btn_callbacks ) cb(button, action, mods);
		});
		glfwSetScrollCallback(window, [](GLFWwindow *window, double xOff, double yOff) {
			for( auto& cb : AssetLib::get().app->scroll_callbacks) cb(xOff, yOff);
		});
		glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xPos, double yPos) {
			for( auto& cb : AssetLib::get().app->cursor_pos_callbacks) cb(xPos, yPos);
		});
		glfwSetDropCallback(window, [](GLFWwindow *window, int count, const char **paths) {
			for( auto& cb : AssetLib::get().app->dnd_callbacks ) cb(count, paths);
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
	}
}





AppBase::~AppBase()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	//ImPlot::DestroyContext();
	ImGui::DestroyContext();

	AppPrefs::get().saveToFile();
	AppPrefs::destroy();
	AssetLib::destroy();

	_resetProfiler();

	glfwDestroyWindow(window);
	glfwTerminate();

	log::pure("\n\n\n");
	log::clear();
}





void AppBase::run()
{
	ImGuiIO& io = ImGui::GetIO();

	while( !glfwWindowShouldClose(window) )
	{
		// delta_time = glm::min(io.DeltaTime, 0.2f);
		delta_time = io.DeltaTime;

		glfwPollEvents();
		static glm::vec2 prevPos(0);
		mouse_off = mouse_pos - prevPos;
		prevPos = mouse_pos;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		ImGuizmo::BeginFrame();

		updateImGui();
		draw_appselector();
		_drawProfilerFps();
		
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

		glFlush();
		glFinish();

		glfwSwapBuffers(window);
	}
}