

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lim/model.h"

const GLuint WIDTH = 800, HEIGHT = 600;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
     std::cout << key << std::endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* win = glfwCreateWindow(WIDTH, HEIGHT, "simplification", NULL, NULL);
    glfwMakeContextCurrent(win);
    if (win == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(win, key_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

       
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
