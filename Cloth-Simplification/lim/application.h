//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. callback list로 호출
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include "scene.h"

namespace lim
{
	class AppBase
	{
	protected:
		struct WindowData
		{
			std::function<void(int width, int height)> win_size_callback;
			std::function<void(int key, int scancode, int action, int mods)> key_callback;
			std::function<void(int button, int action, int mods)> mouse_btn_callback;
			std::function<void(double xOffset, double yOffset)> scroll_callback;
			std::function<void(double xPos, double yPos)> cursor_pos_callback;
			std::function<void(int count, const char** paths)> drop_callback;
		};
		GLFWwindow* window;
		WindowData wData;
		float deltaTime; // sec
		glm::vec4 clear_color ={0.15f, 0.11f, 0.40f, 1.00f};

		GLuint scr_width = 1200;
		GLuint scr_height = 1000;
	protected:
		virtual void update()=0;
	private:
		float lastFrame; // for deltatime
	public:
		AppBase()
		{
			glfwSetErrorCallback(error_callback);
			if( !glfwInit() ) std::exit(-1);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		#endif
			glfwWindowHint(GLFW_SAMPLES, 8); // multisampling sample 3x3
			// glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

			window = glfwCreateWindow(scr_width, scr_height, "simplification", NULL, NULL);
			if( window == NULL )
			{
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				std::exit(-1);
			}

			// window setting
			glfwMakeContextCurrent(window);
			glfwSetWindowUserPointer(window, &wData);
			glfwSwapInterval(1); // vsync??
			// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			initGlfwCallback();

			if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) )
			{
				std::cout << "Failed to initialize GLAD" << std::endl;
				std::exit(-1);
			}
			printVersion();

			// assimp에서 v좌표 반전해서 로딩
			stbi_set_flip_vertically_on_load(true);
			//glEnable(GL_FRAMEBUFFER_SRGB);// match intensity and Voltage todo
			// back face removal
			//glEnable(GL_CULL_FACE);
			//glFrontFace(GL_CCW);
			// wireframe
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//&GL_POINT
		}
		virtual ~AppBase()
		{
			glfwDestroyWindow(window);
			glfwTerminate();
		}
		void run()
		{
			while( !glfwWindowShouldClose(window) )
			{
				float currentFrame = static_cast<float>(glfwGetTime());
				deltaTime = currentFrame - lastFrame;
				lastFrame = currentFrame;

				update();

				glfwPollEvents();
				glfwSwapBuffers(window);
			}
		}
	private:
		void initGlfwCallback()
		{
			glfwSetWindowUserPointer(window, &wData);

			glfwSetWindowSizeCallback(window, [](GLFWwindow* win, int width, int height) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.win_size_callback )
					data.win_size_callback(width, height);
									  });

			glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.key_callback )
					data.key_callback(key, scancode, action, mods);
							   });

			glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.mouse_btn_callback )
					data.mouse_btn_callback(button, action, mods);
									   });

			glfwSetScrollCallback(window, [](GLFWwindow* win, double xOffset, double yOffset) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.scroll_callback )
					data.scroll_callback(xOffset, yOffset);
								  });

			glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xPos, double yPos) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.cursor_pos_callback )
					data.cursor_pos_callback(xPos, yPos);
									 });

			glfwSetDropCallback(window, [](GLFWwindow* win, int count, const char** paths) {
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(win);
				if( data.drop_callback )
					data.drop_callback(count, paths);
								});
		}
		static void error_callback(int error, const char* description)
		{
			fprintf(stderr, "\nGlfw Error %d: %s\n", error, description);
		}
		static void printVersion()
		{
			const GLubyte* renderer = glGetString(GL_RENDERER);
			const GLubyte* vendor = glGetString(GL_VENDOR);
			const GLubyte* version = glGetString(GL_VERSION);
			const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

			GLint major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			printf("GL Vendor            : %s\n", vendor);
			printf("GL Renderer          : %s\n", renderer);
			printf("GL Version (string)  : %s\n", version);
			printf("GLSL Version         : %s\n", glslVersion);
			int nrAttributes;
			glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
			std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl << std::endl;
		}
	};

}

#endif