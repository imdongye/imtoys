//
//  for test simplification and normal map baking.
//  2022-07-21 / im dongye
//
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//
//  TODO List:
//  1. nanovg, nuklear, imgui
//

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Eigen/core>
#define STB_IMAGE_IMPLEMENTATION

#include "lim/program.h"
#include "lim/camera.h"
#include "lim/model.h"
#include "lim/framebuffer.h"
#include "lim/viewport.h"
#include "fqms.h"
//#include "lim/simplify.h"

using namespace std;
using namespace glm;
using namespace lim;
using namespace lim::n_model;

GLuint scr_width = 800;
GLuint scr_height = 600;
vec4 clear_color ={0.15f, 0.11f, 0.40f, 1.00f};

float lastX = scr_width / 2.0f;
float lastY = scr_height / 2.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera;
Framebuffer* framebuffer;
Viewport* viewport;
Program program;
vector<Model*> models;

void init() {
	// assimp에서 v좌표 반전해서 로딩
	stbi_set_flip_vertically_on_load(true);
	//glEnable(GL_FRAMEBUFFER_SRGB);// match intensity and Voltage todo
	// back face removal
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	// wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//&GL_POINT

	camera = Camera(vec3(0.0f, 0.0f, 3.0f), vec3(0, 0, -1), scr_width/(float)scr_height);
	framebuffer = new Framebuffer();
	framebuffer->resize(scr_width, scr_height);
	viewport = new Viewport(framebuffer, &camera);

	program.attatch("shader/diffuse.vs").attatch("shader/diffuse.fs").link();

	models.push_back(new Model([](vector<Vertex>& vertices
					 , vector<GLuint>& indices
					 , vector<Texture>& textures) {
						 const float half = 100.0;
						 vertices.push_back({{-half, 0, half},
											{0, 1, 0}});
						 vertices.push_back({{half, 0, half}, {0, 1, 0}});
						 vertices.push_back({{half, 0, -half}, {0, 1, 0}});
						 vertices.push_back({{-half, 0, -half}, {0, 1, 0}});

						 indices.insert(indices.end(), {0,1,3});
						 indices.insert(indices.end(), {1,2,3});
					 }, "ground"));
	Model& ground = *models.back();

	//models.push_back(new Model("archive/backpack/backpack.obj"));
	models.push_back(new Model("archive/nanosuit/nanosuit.obj"));
	//models.push_back(new Model("archive/tests/armadillo.obj"));
	//models.push_back(new Model("archive/tests/igea.obj"));
	//models.push_back(new Model("archive/tests/teapot.obj"));
	//models.push_back(new Model("archive/tests/stanford-bunny.obj"));
	//models.push_back(new Model("archive/dwarf/Dwarf_2_Low.obj"));
	Model& m = *models.back();
	//-1~1
	m.setUnitScaleAndPivot();
	m.updateModelMat();

	ground.position = vec3(0, -m.getBoundarySize().y*m.scale.y*0.5f, 0);
	ground.updateModelMat();

	//models.push_back(make_unique<Model>(makeQuad(), "ground"));
	fqms::simplifyModel(models.back(), 0.1f);
	models.back()->resetVRAM();
}

void initGUI(GLFWwindow* win) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	float fontSize = 18.0f;// *2.0f;
	//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
	//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(win, true);
	ImGui_ImplOpenGL3_Init("#version 410");
}

void distroy() {
	for( Model* model : models ) {
		delete model;
	}
	program.clear();
	delete viewport;
	delete framebuffer;
}

void render() {

	framebuffer->bind();
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(0, 0, scr_width, scr_height);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);// z-buffer clipping

	GLuint loc, pid;
	pid = program.use();

	loc = glGetUniformLocation(pid, "projMat");
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera.projMat));//&camera.projMat[0][0]);

	loc = glGetUniformLocation(pid, "viewMat"); // also get camera pos here
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera.viewMat));

	loc = glGetUniformLocation(pid, "cameraPos");
	glUniform3fv(loc, 1, value_ptr(camera.position));

	// maybe vpMat
	// and modelMat is declare in model->draw

	for( Model* model : models ) {
		model->draw(program);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShowExampleAppDockSpace() {
// [SECTION] Example App: Docking, DockSpace / ShowExampleAppDockSpace()
//-----------------------------------------------------------------------------

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note: You can use most Docking facilities without calling any API. You DO NOT need to call DockSpace() to use Docking!
// - Drag from window title bar or their tab to dock/undock. Hold SHIFT to disable docking.
// - Drag from window menu button (upper-left button) to undock an entire node (all windows).
// - When io.ConfigDockingWithShift == true, you instead need to hold SHIFT to _enable_ docking/undocking.
// About dockspaces:
// - Use DockSpace() to create an explicit dock node _within_ an existing window.
// - Use DockSpaceOverViewport() to create an explicit dock node covering the screen or a specific viewport.
//   This is often used with ImGuiDockNodeFlags_PassthruCentralNode.
// - Important: Dockspaces need to be submitted _before_ any window they can host. Submit it early in your frame! (*)
// - Important: Dockspaces need to be kept alive if hidden, otherwise windows docked into it will be undocked.
//   e.g. if you have multiple tabs with a dockspace inside each tab: submit the non-visible dockspaces with ImGuiDockNodeFlags_KeepAliveOnly.
// (*) because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node,
// because that window is submitted as part of the part of the NewFrame() call. An easy workaround is that you can create
// your own implicit "Debug##2" window after calling DockSpace() and leave it in the window stack for anyone to use.
	// If you strip some features of, this demo is pretty much equivalent to calling DockSpaceOverViewport()!
	// In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
	// In this specific demo, we are not using DockSpaceOverViewport() because:
	// - we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
	// - we allow the host window to have padding (when opt_padding == true)
	// - we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport() in your code!)
	// TL;DR; this demo is more complicated than what you would normally use.
	// If we removed all the options we are showcasing, this demo would become:
	//     void ShowExampleAppDockSpace()
	//     {
	//         ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
	//     }

	static bool dockspaceOpen = true;
	static bool opt_fullscreen = true;
	static bool opt_padding = false;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if( opt_fullscreen ) {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	} else {
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if( dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode )
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	if( !opt_padding )
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	if( !opt_padding )
		ImGui::PopStyleVar();

	if( opt_fullscreen )
		ImGui::PopStyleVar(2);

	// Submit the DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if( io.ConfigFlags & ImGuiConfigFlags_DockingEnable ) {
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if( ImGui::BeginMenuBar() ) {
		if( ImGui::BeginMenu("Options") ) {
			// Disabling fullscreen would allow the window to be moved to the front of other windows,
			// which we can't undo at the moment without finer window depth/z control.
			ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
			ImGui::MenuItem("Padding", NULL, &opt_padding);
			ImGui::Separator();

			if( ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
			if( ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
			if( ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
			if( ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0) ) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
			if( ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen) ) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
			ImGui::Separator();

			if( ImGui::MenuItem("Close", NULL, false, dockspaceOpen != NULL) )
				dockspaceOpen = false;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::End();
}

void renderImGui() {
	// begin
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGuizmo::BeginFrame();

/////////////////////////////////////////////////////////


	ShowExampleAppDockSpace();

	ImGui::ShowDemoWindow();

	viewport->renderImGui();


/////////////////////////////////////////////////////////
	// end rendering
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)scr_width, (float)scr_height);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Magic!
	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	// For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void processInput(GLFWwindow* win) {
	if( glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS ) {
		float moveSpeed = 1.5f;
		if( glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS )
			moveSpeed = 2.5f;
		if( glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::FORWARD, deltaTime, moveSpeed);
		if( glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::BACKWARD, deltaTime, moveSpeed);
		if( glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::LEFT, deltaTime, moveSpeed);
		if( glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::RIGHT, deltaTime, moveSpeed);
		if( glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::UP, deltaTime, moveSpeed);
		if( glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS )
			camera.move(Camera::MOVEMENT::DOWN, deltaTime, moveSpeed);
	}
}
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mode) {
	if( glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
	   && key == GLFW_KEY_Z ) {
		if( action == GLFW_PRESS ) {
			camera.readyPivot();
			camera.updatePivotViewMat();
		} else if( action == GLFW_RELEASE ) {
			camera.readyFree();
			camera.updateFreeViewMat();
		}
	}
	if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
		glfwSetWindowShouldClose(win, GL_TRUE);
}
void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
	scr_width = width;
	scr_height = height;
}
void cursor_callback(GLFWwindow* win, double xposIn, double yposIn) {
	const GLuint w = scr_width;
	const GLuint h = scr_height;

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	float xoff = xpos - lastX;
	float yoff = lastY - ypos; // inverse

	if( glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) ) {
		if( glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS ) {
			if( glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS ) {
				camera.shiftDist(xoff/w*160.f);
				camera.shiftZoom(yoff/h*160.f);
			} else {
				camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
			}
			camera.updatePivotViewMat();
		} else {
			camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
			camera.updateFreeViewMat();
		}

	}
	lastX = xpos; lastY = ypos;
}
void mouse_btn_callback(GLFWwindow* win, int button, int action, int mods) {
	if( action == GLFW_PRESS ) {
		double xpos, ypos;
		glfwGetCursorPos(win, &xpos, &ypos);
		lastX = xpos;
		lastY = ypos;
	}
}
void scroll_callback(GLFWwindow* win, double xoff, double yoff) {
	camera.shiftZoom(yoff*10.f);
	camera.updateProjMat();
}
void drop_callback(GLFWwindow* win, int count, const char** paths) {
	for( int i = 0; i < count; i++ ) {
		models.push_back(new Model(paths[i]));
		glfwSetWindowTitle(win, models.back()->name.c_str());
	}
}
static void error_callback(int error, const char* description) {
	fprintf(stderr, "\nGlfw Error %d: %s\n", error, description);
}
void printVersion() {
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


int main() {
	glfwSetErrorCallback(error_callback);
	if( !glfwInit() ) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac support 4.1
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	glfwWindowHint(GLFW_SAMPLES, 8); // multisampling sample 3x3

	GLFWwindow* win = glfwCreateWindow(scr_width, scr_height, "simplification", NULL, NULL);
	if( win == NULL ) {
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
	glfwSwapInterval(1); // vsync??

	if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	printVersion();

	init();
	initGUI(win);

	while( !glfwWindowShouldClose(win) ) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(win);

		render();
		//framebuffer->copyToBackBuf();
		renderImGui();

		glfwPollEvents();
		glfwMakeContextCurrent(win);
		glfwSwapBuffers(win);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	distroy();

	glfwDestroyWindow(win);
	glfwTerminate();

	return 0;
}
