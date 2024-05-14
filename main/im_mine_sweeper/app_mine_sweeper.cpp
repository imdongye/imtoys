#include "app_mine_sweeper.h"
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <vector>
#include <glm/gtc/random.hpp>

using namespace std;
using namespace glm;

namespace
{
	struct Cell {
		bool is_open, is_mine, is_flaged;
		int nr_nbrs;
	};
	struct Board {
		ivec2 size;
		vector<vector<Cell>> cells;
		int nr_mine;

		void create(int width, int height, int nrMine)
		{
			nr_mine = nrMine;
			size = {width, height};
			cells.resize(height);
			for( int i=0; i<height; i++ ) {
				cells[i].resize(width);
			}
			for(int cnt=0; cnt<nrMine; cnt++) {
				int x = linearRand(0, width-1);
				int y = linearRand(0, height-1);
				if( cells[y][x].is_mine ) {
					cnt--;
					continue;
				}
				cells[y][x].is_mine = true;
			}
			for( int y=0; y<height; y++ ) for( int x=0; x<width; x++ ) {
				cells[y][x].is_open = false;
				cells[y][x].is_flaged = false;
				if( cells[y][x].is_mine ) 
					continue;
				cells[y][x].nr_nbrs = 0;
				for( int dy=-1; dy<=1; dy++ ) for( int dx=-1; dx<=1; dx++ ) {
					if( dy==0 && dx==0 )
						continue;
					int nx = x+dx;
					int ny = y+dy;
					if( nx<0 || nx>=width || ny<0 || ny>=height )
						continue;
					if( cells[ny][nx].is_mine ) 
						cells[y][x].nr_nbrs++;
				}
			}
		}
	};

	vec2 toGlm(const ImVec2& v) {
		return {v.x, v.y};
	}
	ImVec2 toIG(const vec2& v) {
		return {v.x, v.y};
	}


	Board board;
	const ImFont* font;
}


lim::AppMineSweeper::AppMineSweeper() : AppBase(600, 700, APP_NAME)
{
	board.create(9,9,10);
	ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/SpoqaHanSansNeo-Medium.ttf", 16.f);
	font = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/SpoqaHanSansNeo-Medium.ttf", 50.f);
}
lim::AppMineSweeper::~AppMineSweeper()
{
}
void lim::AppMineSweeper::update() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
void lim::AppMineSweeper::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	//log::drawViewer("logger##mine");

	//
	//	Draw Board
	//
	static ImColor textColors[9] = {
		ImColor::HSV(1.00f,0.00f,1.f),
		ImColor::HSV(0.67f,0.85f,1.f), // 1. blue
		ImColor::HSV(0.41f,0.85f,1.f), // 2. green
		ImColor::HSV(0.01f,0.85f,1.f), // 3. red
		ImColor::HSV(0.01f,0.85f,1.f), // 4
		ImColor::HSV(0.01f,0.85f,1.f), // 5
		ImColor::HSV(0.01f,0.85f,1.f), // 6
		ImColor::HSV(0.01f,0.85f,1.f), // 7
		ImColor::HSV(0.01f,0.85f,1.f), // 8
	};

	ImGui::SetNextWindowSize({600.f, 600.f}, ImGuiCond_Once);
	ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{4, 4});
	ImGui::Begin("board");

	const float paddingRadio = 0.06f;
	const vec2 contentSize = toGlm(ImGui::GetContentRegionAvail());

	const vec2 cellRegion = contentSize/vec2(board.size);
	const vec2 cellSize = cellRegion*vec2(1.f-paddingRadio);
	const float fontSize = glm::min(glm::min(cellSize.x, cellSize.y)*0.9f, 40.f);
	const vec2 cellPadding = cellRegion*vec2(paddingRadio/2.f);
	const float rounding = cellSize.x * 0.1f;

	const vec2 screenPos = toGlm(ImGui::GetCursorScreenPos());
	vec2 mousePos = toGlm(ImGui::GetMousePos()) - screenPos;
	ivec2 hoveredCell = ivec2(mousePos/cellRegion);
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for( int x=0; x<board.size.x; x++ ) for( int y=0; y<board.size.y; y++ ) {
		ImVec2 tlPos = toIG(screenPos + cellPadding + vec2(x, y)*cellRegion);
		ImVec2 brPos = {tlPos.x+cellSize.x, tlPos.y+cellSize.y};
		bool isHovered = hoveredCell.x==x && hoveredCell.y==y;
		const Cell& cell = board.cells[y][x];
		ImColor color = ImColor(0.2f, 0.2f, 0.2f, (isHovered)?0.3f:1.0f);
		if( true||cell.is_open && cell.nr_nbrs>0 ) {
			ImVec2 ftPos;
			ftPos.x = tlPos.x+cellSize.x*0.5f - fontSize*0.23f;
			ftPos.y = tlPos.y+cellSize.y*0.5f - fontSize*0.45f;
			char numStr[2] = {'0'+(char)cell.nr_nbrs, '\0'};
			drawList->AddText(font, fontSize, ftPos, textColors[cell.nr_nbrs], numStr);
			drawList->AddRectFilled(tlPos, brPos, color, rounding);
			//drawList->AddRect(drawPos, drawEndPos, color, rounding, 0, 3.0f);
		}
		else {
			drawList->AddRectFilled(tlPos, brPos, color, rounding);
			if( cell.is_flaged ) {
				
			}

		}
	}
	ImGui::End();
	ImGui::PopStyleVar();

	//
	//	Draw State
	//
	ImGui::Begin("state");
	ImGui::Text("Time: 0:00");
	ImGui::End();
	//
	// Draw Settings
	//

	//
	//	Input
	//
	if( ImGui::IsMouseClicked(0) ) {
		log::pure("asdf");
	}
}