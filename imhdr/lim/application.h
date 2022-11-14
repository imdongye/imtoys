//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
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
	protected:
		struct WindowData // for avoid member func pointer 
		{
			std::vector<std::function<void(int width, int height)>> win_size_callbacks;
			std::vector<std::function<void(int width, int height)>> framebuffer_size_callbacks;
			std::vector<std::function<void(int key, int scancode, int action, int mods)>> key_callbacks;
			std::vector<std::function<void(int button, int action, int mods)>> mouse_btn_callbacks;
			std::vector<std::function<void(double xOff, double yOff)>> scroll_callbacks;
			std::vector<std::function<void(double xPos, double yPos)>> cursor_pos_callbacks;
			std::vector<std::function<void(int count, const char **paths)>> dnd_callbacks;
		};
		virtual void update()=0;
	protected:
		GLFWwindow *window;
		WindowData w_data;
		double delta_time; // sec

		int scr_width;
		int scr_height;
	public:
		/* init */
		AppBase(int _scr_width=1280, int _scr_height=720, const char* title="nonamed")
			:scr_width(_scr_width), scr_height(_scr_height)
		{
			glfwSetErrorCallback(error_callback);
			if( !glfwInit() ) std::exit(-1);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); // mac support 4.1
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		#endif

			window = glfwCreateWindow(scr_width, scr_height, title, NULL, NULL);
			if( window == NULL ) {
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				std::exit(-1);
			}

			/* window setting */
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1);
			// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

			if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				std::exit(-1);
			}

			initGlfwCallbacks();

			printVersion();

			Logger::get()<<"Current path is "<<std::filesystem::current_path().u8string()<<Logger::endl;
			AppPref::get();
		}
		/* destroy */
		virtual ~AppBase()
		{
			AppPref::get().save();
			glfwDestroyWindow(window);
			glfwTerminate();
		}
		void run()
		{
			static double lastTime;

			while( !glfwWindowShouldClose(window) ) {
				float currentFrame = static_cast<float>(glfwGetTime());
				delta_time = currentFrame - lastTime;
				lastTime = currentFrame;

				update();

				glfwPollEvents();
				glfwSwapBuffers(window);
			}
		}
	private:
		void initGlfwCallbacks()
		{
			glfwSetWindowUserPointer(window, &w_data);

			w_data.framebuffer_size_callbacks.push_back([&](int width, int height) {
				scr_width=width; scr_height=height;
			});

			glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for(auto cb : data.win_size_callbacks )
					cb(width, height);
			});

			glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.framebuffer_size_callbacks )
					cb(width, height);
			});

			glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.key_callbacks )
					cb(key, scancode, action, mods);
			});

			glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.mouse_btn_callbacks )
					cb(button, action, mods);
			});

			glfwSetScrollCallback(window, [](GLFWwindow *win, double xOff, double yOff) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.scroll_callbacks)
					cb(xOff, yOff);
			});

			glfwSetCursorPosCallback(window, [](GLFWwindow *win, double xPos, double yPos) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.cursor_pos_callbacks)
					cb(xPos, yPos);
			});

			glfwSetDropCallback(window, [](GLFWwindow *win, int count, const char **paths) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.dnd_callbacks )
					cb(count, paths);
			});
		}
		static void error_callback(int error, const char *description)
		{
			Logger::get().log(stderr, "\nGlfw Error %d: %s\n", error, description);
		}
		void printVersion()
		{
			const GLubyte *renderer = glGetString(GL_RENDERER);
			const GLubyte *vendor = glGetString(GL_VENDOR);
			const GLubyte *version = glGetString(GL_VERSION);
			const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

			GLint major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			Logger::get().log("Frame size           : %d x %d\n", scr_width, scr_height);
			Logger::get().log("GL Vendor            : %s\n", vendor);
			Logger::get().log("GL Renderer          : %s\n", renderer);
			Logger::get().log("GL Version (string)  : %s\n", version);
			Logger::get().log("GLSL Version         : %s\n", glslVersion);
			int nrAttributes;
			glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
			Logger::get() << "Maximum nr of vertex attributes supported: " << nrAttributes << Logger::endl << Logger::endl;
		}
	};
}

#endif