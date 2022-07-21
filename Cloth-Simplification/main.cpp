//
//  for test simplification and normal map baking.
//  2022-07-21 / im dongye
//

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION

#include "lim/program.h"
#include "lim/camera.h"
#include "lim/model.h"

using namespace std;
using namespace glm;

const GLuint INIT_WIDTH = 800, INIT_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = INIT_WIDTH / 2.0f;
float lastY = INIT_HEIGHT / 2.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Program program;
Model model;


void init() {
    program.attatch("shader/diffuse.vs")
        .attatch("shader/diffuse.fs")
        .link();

    model.load("archive/backpack/backpack.obj");
    //model.load("archive/tests/stanford-bunny.obj");
}

void render(GLFWwindow* win) {
    GLuint loc, pid;
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glEnable(GL_FRAMEBUFFER_SRGB);// match intensity and Voltage
    glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);// z-buffer clipping
    glEnable(GL_CULL_FACE);// back face removal
    //glFrontFace(GL_CCW);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    program.use();
    pid = program.ID;

    loc = glGetUniformLocation(pid, "projection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &camera.projMat[0][0]);
    
    loc = glGetUniformLocation(pid, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &camera.viewMat[0][0]);

    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(1.0f, 1.0f, 1.0f));
    
    loc = glGetUniformLocation(pid, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &modelMat[0][0]);

    model.draw(program);

    glfwSwapBuffers(win);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    //std::cout << key << std::endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    camera.aspect = width/(float)height;
    camera.updateProjMat();
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    float xoff = xpos - lastX;
    float yoff = lastY - ypos; // reverse

    if (!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) return;

    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoff, yoff);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

int main() {
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
   glfwWindowHint(GLFW_SAMPLES, 8); // multisampling sample 3x3

    GLFWwindow* win = glfwCreateWindow(INIT_WIDTH, INIT_HEIGHT, "simplification", NULL, NULL);
    if (win == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_callback);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetScrollCallback(win, scroll_callback);
    // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    init();

    while (!glfwWindowShouldClose(win)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(win);

        render(win);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}