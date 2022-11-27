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
			std::vector<std::function<void(int width, int height)>> win_size_callbacks;
			std::vector<std::function<void(int width, int height)>> framebuffer_size_callbacks;
			std::vector<std::function<void(int key, int scancode, int action, int mods)>> key_callbacks;
			std::vector<std::function<void(int button, int action, int mods)>> mouse_btn_callbacks;
			std::vector<std::function<void(double xOff, double yOff)>> scroll_callbacks;
			std::vector<std::function<void(double xPos, double yPos)>> cursor_pos_callbacks;
			std::vector<std::function<void(int count, const char **paths)>> dnd_callbacks;
		};
		WindowData w_data;
		double delta_time; // sec

		int scr_width;
		int scr_height;
	protected:
		virtual void update()=0;
		virtual void renderImGui()=0;
		virtual void keyCallback(int key, int scancode, int action, int mods) {};
		virtual void cursorPosCallback(double xPos, double yPos) {};
		virtual void mouseBtnCallback(int button, int action, int mods) {};
		virtual void scrollCallback(double xOff, double yOff) {};
		virtual void dndCallback(int count, const char **paths) {};
	public:
		/* init */
		AppBase(int _scr_width=1280, int _scr_height=720, const char* title="nonamed")
			:scr_width(_scr_width), scr_height(_scr_height)
		{
			glfwSetErrorCallback([](int error, const char *description) {
				Logger::get(1).log(stderr, "\nGlfw Error %d: %s\n", error, description);
			});
			if( !glfwInit() ) std::exit(-1);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
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


			imgui_modules::initImGui(window);
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
			double lastTime=0.0;
			bool appSelected = false;

			while( !(glfwWindowShouldClose(window) || appSelected) ) {
				float currentFrame = static_cast<float>(glfwGetTime());
				delta_time = currentFrame - lastTime;
				lastTime = currentFrame;

				update();

				imgui_modules::beginImGui();
				renderImGui();

				appSelected = imgui_modules::draw_appselector();

				imgui_modules::endImGui(scr_width, scr_height);

				glfwPollEvents();
				glfwSwapBuffers(window);
			}
		}
	private:
		void registerCallbacks()
		{
			glfwSetWindowUserPointer(window, &w_data);

			/* lambda is better then std::bind */
			w_data.key_callbacks.push_back([this](int key, int scancode, int action, int mods) {
				keyCallback(key, scancode, action, mods); });
			w_data.mouse_btn_callbacks.push_back([this](int button, int action, int mods){
				mouseBtnCallback(button, action, mods); });
			w_data.scroll_callbacks.push_back([this](double xOff, double yOff) {
				scrollCallback(xOff, yOff); });
			w_data.cursor_pos_callbacks.push_back([this](double xPos, double yPos) { 
				cursorPosCallback(xPos, yPos); });
			w_data.dnd_callbacks.push_back([this](int count, const char **path) { 
				dndCallback(count, path); });

			w_data.framebuffer_size_callbacks.push_back([this](int width, int height) {
				scr_width=width; scr_height=height;
			});
		}
		void initGlfwCallbacks()
		{
			registerCallbacks();

			glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for(auto cb : data.win_size_callbacks ) if(cb) cb(width, height);
				printf("win %d %d\n", width, height);
			});
			glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.framebuffer_size_callbacks ) cb(width, height);
			});
			glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.key_callbacks ) cb(key, scancode, action, mods);
			});
			glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.mouse_btn_callbacks ) cb(button, action, mods);
			});
			glfwSetScrollCallback(window, [](GLFWwindow *win, double xOff, double yOff) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.scroll_callbacks) cb(xOff, yOff);
			});
			glfwSetCursorPosCallback(window, [](GLFWwindow *win, double xPos, double yPos) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.cursor_pos_callbacks) cb(xPos, yPos);
			});
			glfwSetDropCallback(window, [](GLFWwindow *win, int count, const char **paths) {
				WindowData &data = *(WindowData*)glfwGetWindowUserPointer(win);
				for( auto cb : data.dnd_callbacks ) cb(count, paths);
			});
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
			Logger::get().log("Maximum nr of vertex attributes supported: %d\n", nrAttributes);
		}
	};
}

#endif
