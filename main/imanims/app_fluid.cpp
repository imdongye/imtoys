#include "app_fluid.h"
#include "canvas.h"
#include <stb_image.h>
#include <imgui.h>

namespace
{
	lim::CanvasColor* canvas;
}

namespace lim
{
	AppFluid::AppFluid(): AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);

		//vp = new Viewport(new Framebuffer(),900, 600);
		//fb = vp->framebuffer;
		// 람다에서 전체 캡쳐를 해두어도 컴파일타임에 사용하는 변수만 복사 및 레퍼런싱한다.
		//vp->resize_callback = [this](int w, int h) { canvas->resize(w, h); };

		canvas = new CanvasColor(fb_width, fb_height);
		points.push_back({0,0});
	}
	AppFluid::~AppFluid()
	{
		//delete vp;
		delete canvas;
	}
	void AppFluid::update()
	{
		using namespace glm;
		canvas->clear();
		points[0] = mouse_pos;
		points[0].y = fb_height-1-points[0].y;

		const float rad = 50.f;
		const float threshold = 0.5f;

		for( int i=0; i<fb_height; i++ ) for( int j=0; j<fb_width; j++ ) {
			glm::vec2 pos = {j, i};
			float sum = 0.f;

			for( auto& pt : points ) {
				glm::vec2 dif = pt-pos;
				float dd = glm::dot(dif, dif);
				float rr = rad*rad;

				if( dd < rr ) {
					sum += pow(1.f-pow(dd/rr, 2), 2);
				}				
			}

			if( sum>threshold ) {
				canvas->at(pos.x, pos.y) = glm::vec3(1.);
			}
		}

		/*glBindTexture(GL_TEXTURE_2D, fb->color_tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb->width, fb->height, GL_FLOAT, GL_RGB, canvas->M.data());
		glBindTexture(GL_TEXTURE_2D, 0);*/

		canvas->update();

		Tex2Fbo(canvas->tex_id, 0, fb_width, fb_height);
	}
	void AppFluid::renderImGui()
	{
		//ImGui::DockSpaceOverViewport();

		if( ImGui::BeginMainMenuBar() ) {
			if( ImGui::BeginMenu("File") ) {
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Edit") ) {
				if( ImGui::MenuItem("Undo", "CTRL+Z") ) {}
				if( ImGui::MenuItem("Redo", "CTRL+Y", false, false) ) {}  // Disabled item
				ImGui::Separator();
				if( ImGui::MenuItem("Cut", "CTRL+X") ) {}
				if( ImGui::MenuItem("Copy", "CTRL+C") ) {}
				if( ImGui::MenuItem("Paste", "CTRL+V") ) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		{
			static bool pOpen = true;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing 
				| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

			const float PAD = 10.0f;
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = viewport->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = work_pos.x + PAD;
			window_pos.y = work_pos.y + PAD;
			window_pos_pivot.x = 0.0f;
			window_pos_pivot.y = 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

			ImGui::SetNextWindowBgAlpha(0.35f);
			ImGui::Begin("Example: Simple overlay", &pOpen, window_flags);
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("mouse pos : %4d, %4d", mouse_pos.x, mouse_pos.y);
			ImGui::End();
		}
		//vp->drawImGui();
	}
	void AppFluid::framebufferSizeCallback(int w, int h)
	{
		canvas->resize(w, h);
	}
	void AppFluid::mouseBtnCallback(int button, int action, int mods)
	{
		if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			points.push_back({mouse_pos.x, fb_height-1-mouse_pos.y});
		}
	}
	void AppFluid::cursorPosCallback(double xPos, double yPos)
	{
		//if( !vp->dragging ) return;
		if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ) {
			canvas->at(mouse_pos.x,fb_height-1-mouse_pos.y) = {0.5,1,0.5};
		}
	}
}