//
//  for test simplification and normal map baking.
//  2022-07-21 / im dongye
//
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION

#include "lim/program.h"
#include "lim/camera.h"
#include "lim/model.h"
#include "lim/simplify.h"

using namespace std;
using namespace glm;

const GLuint SCR_WIDTH = 800, SCR_HEIGHT = 600;

// anonymous namespace for 외부연결누수
namespace {
// camera
Camera camera(SCR_WIDTH/(float)SCR_HEIGHT, glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Program program;
vector<unique_ptr<Model>> models;


void makeGround(vector<Vertex>& vertices
            , vector<GLuint>& indices
            , vector<Texture>& textures) {
    const float half = 4.0;
    vertices.push_back({{-half, 0, half}, {0, 1, 0}});
    vertices.push_back({{half, 0, half}, {0, 1, 0}});
    vertices.push_back({{half, 0, -half}, {0, 1, 0}});
    vertices.push_back({{-half, 0, -half}, {0, 1, 0}});

    indices.insert(indices.end(), {0,1,3});
    indices.insert(indices.end(), {1,2,3});
}
void makeTriangle(vector<Vertex>& vertices
                 , vector<GLuint>& indices
                 , vector<Texture>& textures) {
    vertices.push_back({{-0.5f, -0.5f, 0.f}});
    vertices.push_back({{0.5f, -0.5f, 0.f}});
    vertices.push_back({{0.0, 0.5, 0.f}});
    
    indices.insert(indices.end(), {0,1,2});
}

void init() {
    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);// match intensity and Voltage
    // back face removal
    //glEnable(GL_CULL_FACE);
    //glFrontFace(GL_CCW);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//GL_POINT
    
    program.attatch("shader/diffuse.vs").attatch("shader/diffuse.fs").link();
    models.push_back(make_unique<Model>("archive/backpack/backpack.obj"));
    //models.push_back(make_unique<Model>("archive/tests/igea.obj"));
    //models.push_back(make_unique<Model>("archive/tests/stanford-bunny.obj"));
    //models.push_back(make_unique<Model>(makeGround, "ground"));
    //models.back()->position = vec3(0,-4,0);
    //models.push_back(make_unique<Model>(makeQuad(), "ground"));
}

void render(GLFWwindow* win) {
    glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);// z-buffer clipping
    
    
    ////////////////////////
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    GLuint loc, pid;
    pid = program.use();

    loc = glGetUniformLocation(pid, "projMat");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera.projMat));//&camera.projMat[0][0]);
    
    loc = glGetUniformLocation(pid, "viewMat");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera.viewMat));

    loc = glGetUniformLocation(pid, "cameraPos");
    glUniform3fv(loc, 1, value_ptr(camera.position));
    
    // maybe vpMat
    // modelMat is declare in model->draw

    for (auto& model : models) {
        model->draw(program);
    }
    ////////////////////////


    glfwSwapBuffers(win);
}

void processInput(GLFWwindow *win) {
    if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS) {
        float moveSpeed = 1.5f;
        if (glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) 
            moveSpeed = 2.5f;
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
            camera.move(FORWARD, deltaTime, moveSpeed);
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
            camera.move(BACKWARD, deltaTime, moveSpeed);
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
            camera.move(LEFT, deltaTime, moveSpeed);
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
            camera.move(RIGHT, deltaTime, moveSpeed);
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
            camera.move(UP, deltaTime, moveSpeed);
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
            camera.move(DOWN, deltaTime, moveSpeed);
    }
}
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mode) {
    if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS 
        && key == GLFW_KEY_Z) {
        if(action == GLFW_PRESS) {
            camera.readyPivot();
            camera.updatePivotViewMat();
        }
        else if(action == GLFW_RELEASE) {
            camera.readyFree();
            camera.updateFreeViewMat();
        }
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GL_TRUE);
}
void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
    glViewport(0, 0, width, height);
    camera.aspect = width/height;
}
void cursor_callback(GLFWwindow* win, double xposIn, double yposIn) {
    int w, h;
    glfwGetWindowSize(win, &w, &h);
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    float xoff = xpos - lastX;
    float yoff = lastY - ypos; // inverse

    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)) {
        if(glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS) {
            if(glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS) {
                camera.shiftDist(xoff/w*160.f);
                camera.shiftZoom(yoff/h*160.f);
            }
            else {
                camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
            }
            camera.updatePivotViewMat();
        }
        else {
            camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
            camera.updateFreeViewMat();
        }
        
    }
    camera.updateProjMat();
    lastX = xpos; lastY = ypos;
}
void mouse_btn_callback(GLFWwindow* win, int button, int action, int mods) {
    if(action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(win, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
    }
}
void scroll_callback(GLFWwindow* win, double xoff, double yoff) {
    camera.shiftZoom(yoff);
    camera.updateProjMat();
}
void drop_callback(GLFWwindow* win, int count, const char** paths)
{
    for (int i = 0; i < count; i++) {
        models.push_back(make_unique<Model>(paths[i]));
        glfwSetWindowTitle(win, models.back()->name.c_str());
    }
}
void printVersion() {
    const GLubyte *renderer = glGetString( GL_RENDERER );
    const GLubyte *vendor = glGetString( GL_VENDOR );
    const GLubyte *version = glGetString( GL_VERSION );
    const GLubyte *glslVersion =
           glGetString( GL_SHADING_LANGUAGE_VERSION );
     
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
     
    printf("GL Vendor            : %s\n", vendor);
    printf("GL Renderer          : %s\n", renderer);
    printf("GL Version (string)  : %s\n", version);
    printf("GLSL Version         : %s\n\n", glslVersion);
}
} // anonymouse namespace


int main() {
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 8); // multisampling sample 3x3

    GLFWwindow* win = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "simplification", NULL, NULL);
    if (win == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // window setting
    // glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_callback);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback(win, cursor_callback);
    glfwSetMouseButtonCallback(win, mouse_btn_callback);
    glfwSetScrollCallback(win, scroll_callback);
    glfwSetDropCallback(win, drop_callback);
    // glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    printVersion();
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
