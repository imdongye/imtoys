//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
// 
//	callback을 window data에 직접 추가할수도있고 가상함수는 기본으로 등록되어있어서 오버라이딩하기만 하면됨.
//	상속한 앱에서 등록하는 코드가 없어서 깔끔함. 하지만 가상함수테이블에서 포인팅되는과정이 성능에 문제될수있다.
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. glfw에서 함수 이름 스네이크/카멜 혼용 이유 찾기
//  3. glfwSwapInterval(1); // vsync??
//

#ifndef APPLICATION_H
#define APPLICATION_H

namespace lim
{
	class AppBase
	{
	public:
		GLFWwindow *window;
		
		struct WindowData // for avoid member func pointer 
		{
			Callbacks<void(int width, int height)> win_size_callbacks;
			Callbacks<void(int width, int height)> framebuffer_size_callbacks;
			Callbacks<void(int key, int scancode, int action, int mods)> key_callbacks;
			Callbacks<void(int button, int action, int mods)> mouse_btn_callbacks;
			Callbacks<void(double xOff, double yOff)> scroll_callbacks;
			Callbacks<void(double xPos, double yPos)> cursor_pos_callbacks;
			Callbacks<void(int count, const char **paths)> dnd_callbacks;
			Callbacks<void(float deltaTime)> update_hooks;
		};
		WindowData w_data;
		
		double delta_time; // sec

		// relative to ratina or window monitor setting
		int win_width, win_height;
		// real pixel coordinate
		int fb_width, fb_height;
		float aspect_ratio; // width/height;
		float pixel_ratio;    // (DPI)
		glm::vec2 mouse_pos;

	protected:
		virtual void update()=0;
		virtual void renderImGui()=0;
		virtual void framebufferSizeCallback(int w, int h) {};
		virtual void keyCallback(int key, int scancode, int action, int mods) {};
		virtual void cursorPosCallback(double xPos, double yPos) {};
		virtual void mouseBtnCallback(int button, int action, int mods) {};
		virtual void scrollCallback(double xOff, double yOff) {};
		virtual void dndCallback(int count, const char **paths) {};
	public:
		/* init */
		AppBase(int winWidth=1280, int winHeight=720, const char* title="nonamed")
			:win_width(winWidth), win_height(winHeight)
		{
			glfwSetErrorCallback([](int error, const char *description) {
				Logger::get(1).log(stderr, "\nGlfw Error %d: %s\n", error, description);
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
			glfwGetFramebufferSize(window, &fb_width, &fb_height);
			aspect_ratio = fb_width/(float)fb_height;
			pixel_ratio =  fb_width/(float)win_width;

			if( window == NULL ) {
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				std::exit(-1);
			}

			/* window setting */
			// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
			int nrMonitors;
			GLFWmonitor** monitors = glfwGetMonitors(&nrMonitors);
			const GLFWvidmode* vidMode = glfwGetVideoMode(monitors[0]);

			glfwSetWindowPos(window, (vidMode->width-win_width)*0.5f, (vidMode->height-win_height)*0.5f);
			glfwShowWindow(window);


			glfwMakeContextCurrent(window);
			// default is 0 then make tearing so must 1 to buffer swap when all buffer is showen
			glfwSwapInterval(1); // vsync

			if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				std::exit(-1);
			}

			// register callback after glad initialization
			initGlfwCallbacks();

			printVersionAndStatus();

			imgui_modules::initImGui(window);

			// need for restart app
			AppPref::get();
			Viewport::id_generator=0;
			Logger::get().windowName = "Logger##log"+AppPref::get().selectedAppName;
			AssetLib::reload();
			Scene::sceneCounter=0;
		}
		/* destroy */
		virtual ~AppBase()
		{
			imgui_modules::destroyImGui();

			AppPref::get().save();
			glfwDestroyWindow(window);
			glfwTerminate();
		}
		void run()
		{
			double lastTime=glfwGetTime();

			while( !glfwWindowShouldClose(window) ) {
				double currentTime = glfwGetTime();
				delta_time = currentTime - lastTime;
				lastTime = currentTime;

				update();
				for( auto& [_, cb] : w_data.update_hooks ) 
					cb(delta_time);

				imgui_modules::beginImGui();

				renderImGui();

				imgui_modules::draw_appselector();

				imgui_modules::endImGui(win_width, win_height);

				glfwPollEvents();
				glfwSwapBuffers(window);
			}
		}
	private:
		void initGlfwCallbacks()
		{
			glfwSetWindowUserPointer(window, &w_data);

			/* lambda is better then std::bind */

			// first call when resize window
			w_data.framebuffer_size_callbacks[this] =[this](int w, int h) {
				fb_width=w; fb_height=h;
				framebufferSizeCallback(w, h);
			};
			// second call when resize window
			w_data.win_size_callbacks[this] = [this](int width, int height) {
				win_width=width; win_height=height;
				aspect_ratio = fb_width/fb_height;
				pixel_ratio = fb_width/win_width;
			};			w_data.key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
				keyCallback(key, scancode, action, mods); 
			};
			w_data.mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
				mouseBtnCallback(button, action, mods); 
			};
			w_data.scroll_callbacks[this] = [this](double xOff, double yOff) {
				scrollCallback(xOff, yOff); 
			};
			w_data.cursor_pos_callbacks[this] = [this](double x, double y) {
				mouse_pos = {x,y};
				cursorPosCallback(x, y); 
			};
			w_data.dnd_callbacks[this] = [this](int count, const char **path) {
				dndCallback(count, path); 
			};

			glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for(auto& [_, cb] : data.win_size_callbacks ) cb(width, height);
			});
			glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.framebuffer_size_callbacks ) cb(width, height);
			});
			glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.key_callbacks ) cb(key, scancode, action, mods);
			});
			glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.mouse_btn_callbacks ) cb(button, action, mods);
			});
			glfwSetScrollCallback(window, [](GLFWwindow *window, double xOff, double yOff) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.scroll_callbacks) cb(xOff, yOff);
			});
			glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xPos, double yPos) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.cursor_pos_callbacks) cb(xPos, yPos);
			});
			glfwSetDropCallback(window, [](GLFWwindow *window, int count, const char **paths) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(window);
				for( auto& [_, cb] : data.dnd_callbacks ) cb(count, paths);
			});
		}

		void printVersionAndStatus()
		{
			const GLubyte *renderer = glGetString(GL_RENDERER);
			const GLubyte *vendor = glGetString(GL_VENDOR);
			const GLubyte *version = glGetString(GL_VERSION);
			const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

			GLint major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			Logger::get().log("Frame size           : %d x %d\n", win_width, win_height);
			Logger::get().log("GL Vendor            : %s\n", vendor);
			Logger::get().log("GL Renderer          : %s\n", renderer);
			Logger::get().log("GL Version (string)  : %s\n", version);
			Logger::get().log("GLSL Version         : %s\n", glslVersion);
			int nrAttributes, nrTextureUnits;
			glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
			Logger::get().log("Maximum nr of vertex attributes supported: %d\n", nrAttributes);
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &nrTextureUnits);
			Logger::get().log("Maximum nr of texture slots supported: %d\n", nrTextureUnits);

			Logger::get()<<"Current path is "<<std::filesystem::current_path().u8string()<<Logger::endl<<Logger::endl;
		}
	};
}

#endif
