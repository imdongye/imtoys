//
//  for astar visualize
//	2023-03-03 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_ASTAR_H
#define APP_ASTAR_H

#include "path_finder.h"

namespace lim
{
	class AppAstar: public AppBase
	{
	public:
        inline static constexpr const char *APP_DIR = "imtests/";
        inline static constexpr const char *APP_NAME = "astar visualizer";
		inline static constexpr const char *APP_DISC = "left click: destination, right click: start position";
	private:
		PathFinder::NodeState draw_mode;
		ImColor state_colors[7];
		int width, height;
		PathFinder path_finder;
		glm::ivec2 hover_pos;
	public:
		AppAstar(): AppBase(900, 1050, APP_NAME)
			, width(18), height(18), path_finder(width, height)
		{
			stbi_set_flip_vertically_on_load(true);
			draw_mode = PathFinder::NS_WALL;

			state_colors[PathFinder::NS_ROAD] = ImColor(0.2f, 0.2f, 0.2f); // gray not filled
			state_colors[PathFinder::NS_OPEN] = ImColor(0.7f, 0.6f, 0.1f, 0.8f); // 
			state_colors[PathFinder::NS_CLOSE] = ImColor(0.9f, 0.9f, 0.2f, 0.8f); // 
			state_colors[PathFinder::NS_WALL] = ImColor(0.4f, 0.4f, 0.4f); // dark gray
			state_colors[PathFinder::NS_START] = ImColor(0.1f, 0.2f, 1.0f); // blue
			state_colors[PathFinder::NS_END] = ImColor(1.0f, 0.2f, 0.1f); // red
			state_colors[PathFinder::NS_PATH] = ImColor(6.0f, 0.1f, 0.6f); // purple
		}
		~AppAstar()
		{

		}
	private:
		virtual void update() final
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		void drawMap()
		{
			ImGui::SetNextWindowSize({850.f, 850.f}, ImGuiCond_Once);
			ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
			ImGui::Begin("map");

			const float paddingRadio = 0.06;
			const ImVec2 contentSize = ImGui::GetContentRegionAvail();

			const ImVec2 nodeRegion ={contentSize.x/(float)width,
									  contentSize.y/(float)height};

			const ImVec2 nodeSize ={nodeRegion.x*(1.0f-paddingRadio),
							  nodeRegion.y*(1.0f-paddingRadio)};

			const float rounding = nodeSize.x * 0.1f;

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const ImVec2 screenPos = ImGui::GetCursorScreenPos();
			ImVec2 mousePos = ImGui::GetMousePos();
			mousePos ={mousePos.x-screenPos.x, mousePos.y-screenPos.y};
			hover_pos ={(int)(mousePos.x/nodeRegion.x), (int)(mousePos.y/nodeRegion.y)};
			ImVec2 drawPos = screenPos;

			for( int i=0; i<height; i++ ) {
				drawPos.x = screenPos.x;
				for( int j=0; j<width; j++ ) {
					bool isHovered = false;
					ImVec2 drawEndPos ={drawPos.x+nodeSize.x,drawPos.y+nodeSize.y};
					PathFinder::Node& curNode = path_finder.map[i][j];
					ImU32 color = state_colors[curNode.state];

					if( hover_pos.x == j && hover_pos.y == i ) {
						ImColor forAlpha = ImColor(state_colors[draw_mode]);
						forAlpha.Value.w = 0.3f;
						color = forAlpha;
						isHovered = true;
						if( draw_mode==PathFinder::NS_ROAD ) {
							drawList->AddRect(drawPos, drawEndPos, color, rounding, 0, 3.0f);
						} else {
							drawList->AddRectFilled(drawPos, drawEndPos, color, rounding);
						}
					}
					else {
						if( curNode.state==PathFinder::NS_ROAD ) {
							drawList->AddRect(drawPos, drawEndPos, color, rounding, 0, 3.0f);
						} else {
							drawList->AddRectFilled(drawPos, drawEndPos, color, rounding);
						}
					}
					drawPos.x += nodeRegion.x;
				}
				drawPos.y += nodeRegion.y;
			}

			ImGui::End();
		}
		void drawController()
		{
			ImGui::Begin("map controller");
			ImGui::RadioButton("wall", (int*)&draw_mode, (int)PathFinder::NS_WALL); ImGui::SameLine();
			ImGui::RadioButton("road", (int*)&draw_mode, (int)PathFinder::NS_ROAD); ImGui::SameLine();
			ImGui::RadioButton("start", (int*)&draw_mode, (int)PathFinder::NS_START); ImGui::SameLine();
			ImGui::RadioButton("destination", (int*)&draw_mode, (int)PathFinder::NS_END);
			if( ImGui::Button("find") ) {
				path_finder.updatePath();
			}
			ImGui::SameLine();
			if( path_finder.path.size() > 0)
				ImGui::Text("path length: %d", path_finder.path.size());
			else
				ImGui::Text("path not found");

			if( ImGui::SliderInt("width", &width, 10, 30)||ImGui::SliderInt("height", &height, 10, 30) ) {
				path_finder.resizeMap(width, height);
			}
			ImGui::End();
		}
		virtual void renderImGui() final
		{
			imgui_modules::ShowExampleAppDockSpace([]() {});

			//ImGui::ShowDemoWindow();
			
			drawMap();
			drawController();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
		}
		virtual void cursorPosCallback(double xPos, double yPos) final
		{
			static glm::ivec2 prevHoverPos={-1,-1};
			if( hover_pos!=prevHoverPos && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS ) {
				path_finder.setMapPos(hover_pos.x, hover_pos.y, draw_mode);
			}
			prevHoverPos=hover_pos;
		}
		virtual void mouseBtnCallback(int button, int action, int mods) final
		{
			if( button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS ) {
				path_finder.setMapPos(hover_pos.x, hover_pos.y, draw_mode);
			}
		}
	};
}

#endif
