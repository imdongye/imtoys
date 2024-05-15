#include "app_mine_sweeper.h"
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <glm/gtc/random.hpp>

using namespace std;
using namespace glm;
using namespace lim;

namespace {
	vec2 toGlm(const ImVec2& v) {
		return {v.x, v.y};
	}
	ImVec2 toIG(const vec2& v) {
		return {v.x, v.y};
	}
	constexpr int TOP_WINDOW_HEIGHT = 100;
	constexpr int NINE_BOARD_WINDOW_SIZE = 380;
	constexpr float UI_FONT_SIZE = 20.f;
	constexpr float CELL_FONT_SIZE = 30.f;
	ImFont* g_ui_font;
	ImFont* g_cell_font;
}




bool AppMineSweeper::Board::isOutside(int x, int y) {
	return ( x<0 || x>=width || y<0 || y>=height );
}
void AppMineSweeper::Board::clear() {
	for( int y=0; y<MAX_BOARD_SIZE; y++ ) for( int x=0; x<MAX_BOARD_SIZE; x++ ) {
		Cell& c = cells[y][x];
		c.is_open = false;
		c.is_mine = false;
		c.is_flaged = false;
		c.nr_nbrs = 0;
	}
}
void AppMineSweeper::Board::plantMines(int avoidX, int avoidY) {
	nr_closed = width*height;
	nr_flagged = 0;

	for(int cnt=0; cnt<nr_mine; cnt++) {
		int x = linearRand(0, width-1);
		int y = linearRand(0, height-1);
		bool insideAvoid = avoidX-1<=x && x<=avoidX+1
						&& avoidY-1<=y && y<=avoidY+1;
		if( cells[y][x].is_mine || insideAvoid ) {
			cnt--;
			continue;
		}
		cells[y][x].is_mine = true;
	}

	for( int y=0; y<height; y++ ) for( int x=0; x<width; x++ ) {
		cells[y][x].nr_nbrs = 0;
		if( cells[y][x].is_mine ) 
			continue;
		for( int dy=-1; dy<=1; dy++ ) for( int dx=-1; dx<=1; dx++ ) {
			if( dy==0 && dx==0 )
				continue;
			int nx = x+dx;
			int ny = y+dy;
			if( isOutside(nx, ny) )
				continue;
			if( cells[ny][nx].is_mine ) 
				cells[y][x].nr_nbrs++;
		}
	}
}
// if mine return true
bool AppMineSweeper::Board::reveal(int x, int y) {
	Cell& cell = cells[y][x];
	if( isOutside(x, y) || cell.is_open )
		return false;
	cell.is_open = true;
	nr_closed--;
	if( cell.is_flaged ) 
		switchFlag(x,y);

	if( cell.nr_nbrs==0 && !cell.is_mine ) {
		reveal(x-1, y);
		reveal(x+1, y);
		reveal(x, y-1);
		reveal(x, y+1);
	}
	return cell.is_mine;
}
void AppMineSweeper::Board::switchFlag(int x, int y) {
	if( isOutside(x, y) )
		return;
	Cell& cell = cells[y][x];
	cell.is_flaged = !cell.is_flaged;
	nr_flagged += cell.is_flaged ? 1 : -1;
}








void AppMineSweeper::setNewGame() {
	game_state = GameState::NEW;
	elapsed_time = 0.0;
	board.clear();
	board.width = adj.width;
	board.height = adj.height;
	board.nr_mine = adj.nr_mine;
}

AppMineSweeper::AppMineSweeper() 
	: AppBase(NINE_BOARD_WINDOW_SIZE, NINE_BOARD_WINDOW_SIZE+TOP_WINDOW_HEIGHT, APP_NAME)
{
	setNewGame();

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("assets/fonts/SpoqaHanSansNeo-Medium.ttf", 16.f
								, nullptr, io.Fonts->GetGlyphRangesKorean());
	
	ImFontConfig config;
    // config.GlyphMinAdvanceX = 13.0f;
	config.MergeMode = true;
    static ImWchar lIconRanges[] = { 0xE800, 0xE810, 0 };


	io.Fonts->AddFontFromFileTTF("assets/fonts/SpoqaHanSansNeo-Medium.ttf", UI_FONT_SIZE
								, nullptr, io.Fonts->GetGlyphRangesKorean());
	g_ui_font = io.Fonts->AddFontFromFileTTF("im_mine_sweeper/fontello/font/fontello.ttf"
											, UI_FONT_SIZE, &config, lIconRanges);


	io.Fonts->AddFontFromFileTTF("assets/fonts/SpoqaHanSansNeo-Medium.ttf", CELL_FONT_SIZE);
    g_cell_font = io.Fonts->AddFontFromFileTTF("im_mine_sweeper/fontello/font/fontello.ttf"
											  , CELL_FONT_SIZE, &config, lIconRanges);

}
AppMineSweeper::~AppMineSweeper()
{
}
void AppMineSweeper::update() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
void AppMineSweeper::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	//log::drawViewer("logger##mine");

	//
	//	Board
	//
	const int COL_CELL_IDX = 0;
	const int COL_MINE_IDX = 9;
	const int COL_FLAG_RIGHT_IDX = 10;
	const int COL_FLAG_WRONG_IDX = 11;
	static ImColor boardColors[12] = {
		ImColor::HSV(1.00f,0.00f,0.5f, 0.8f),// 0. cell 
		ImColor::HSV(0.67f,0.85f,1.f), // 1. blue
		ImColor::HSV(0.41f,0.85f,1.f), // 2. green
		ImColor::HSV(0.01f,0.75f,1.f), // 3. red
		ImColor::HSV(0.01f,0.75f,1.f), // 4
		ImColor::HSV(0.01f,0.75f,1.f), // 5
		ImColor::HSV(0.01f,0.75f,1.f), // 6
		ImColor::HSV(0.01f,0.75f,1.f), // 7
		ImColor::HSV(0.01f,0.75f,1.f), // 8
		ImColor::HSV(0.01f,0.75f,1.f), // 9  mine 
		ImColor::HSV(0.01f,0.75f,1.f), // 10 flag right
		ImColor::HSV(0.67f,0.85f,1.f), // 11 flag wrong

	};

	ImGui::SetNextWindowSize({600.f, 600.f}, ImGuiCond_Once);
	ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{4, 4});
	ImGui::Begin("board");
	const bool isBoardHovered = ImGui::IsWindowHovered();
	const float paddingRadio = 0.06f;
	const vec2 contentSize = toGlm(ImGui::GetContentRegionAvail());

	const vec2 cellRegion = contentSize/vec2(board.width, board.height);
	const vec2 cellSize = cellRegion*vec2(1.f-paddingRadio);
	const float fontSize = glm::min(glm::min(cellSize.x*0.9f, cellSize.y)*0.8f, CELL_FONT_SIZE);
	const vec2 cellPadding = cellRegion*vec2(paddingRadio/2.f);
	const float rounding = cellSize.x * 0.1f;


	const vec2 screenPos = toGlm(ImGui::GetCursorScreenPos());
	vec2 mousePos = toGlm(ImGui::GetMousePos()) - screenPos;
	int hoveredX = (mousePos.x>0)? mousePos.x/cellRegion.x : -1;
	int hoveredY = (mousePos.y>0)? mousePos.y/cellRegion.y : -1;
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	for( int y=0; y<board.height; y++ ) for( int x=0; x<board.width; x++ ) {
		const Cell& cell = board.cells[y][x];
		bool isHovered = hoveredX==x && hoveredY==y;
		ImVec2 tlPos = toIG(screenPos + cellPadding + vec2(x, y)*cellRegion);
		ImVec2 brPos = {tlPos.x+cellSize.x, tlPos.y+cellSize.y};
		ImColor col = boardColors[COL_CELL_IDX];

		if( cell.is_open ) {
			drawList->AddRect(tlPos, brPos, col, rounding);
			if( !cell.is_mine && cell.nr_nbrs==0 ) {
				continue;
			}
		}
		else { // closed
			if( isHovered || (game_state == GameState::OVER && cell.is_mine) ) {
				col.Value.w = 0.3f;
			}
			drawList->AddRectFilled(tlPos, brPos, col, rounding);
			if( game_state == GameState::OVER ) {
				if( !cell.is_flaged && !cell.is_mine)
					continue;
			}
			else {
				if( !cell.is_flaged )
					continue;
			}
		}

		const char* txPtr;
		ImVec2 txPos;
		txPos.x = tlPos.x+cellSize.x*0.5f - fontSize*0.23f;
		txPos.y = tlPos.y+cellSize.y*0.5f - fontSize*0.45f;

		if( cell.is_flaged ) {
			txPos.x -= fontSize*0.16f;
			col = boardColors[(game_state==GameState::OVER&&cell.is_mine)?COL_FLAG_RIGHT_IDX:COL_FLAG_WRONG_IDX];
			txPtr = u8"\uE800";
		} 
		else if( cell.is_mine ) {
			if( game_state == GameState::PLAYING && !cell.is_open )
				continue;
			txPos.x -= fontSize*0.16f;
			col = boardColors[COL_MINE_IDX];
			txPtr = u8"\uE802";
		}
		else {
			static char numStr[2] = {0,};
			numStr[0] = (char)(cell.nr_nbrs+'0');
			col = boardColors[cell.nr_nbrs];
			txPtr = numStr;
		}
		drawList->AddText(g_cell_font, fontSize, txPos, col, txPtr);
	}
	ImGui::End();
	ImGui::PopStyleVar();



	//
	//	State
	//
	if( game_state != GameState::NEW ) {
		ImGui::Begin("state");
		ImGui::PushFont(g_ui_font);

		if( game_state == GameState::PLAYING ) {
			elapsed_time = ImGui::GetTime() - start_time;
		}
		int eSec = (int)elapsed_time;
		if( game_state==GameState::OVER ) {
			ImGui::Text(( board.nr_closed == board.nr_mine )?u8"클리어!":u8"게임오버");
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"\uE803 %d:%02d", eSec/60, eSec%60);
		ImGui::SetItemTooltip("%.3f sec", elapsed_time);

		ImGui::SameLine(0.f,15.f);
		int remainMines = glm::min(board.nr_closed, board.nr_mine-board.nr_flagged);
		ImGui::Text(u8"\uE802 %d/%d", remainMines, board.nr_mine);
		if( game_state != GameState::NEW ) {
			ImGui::SameLine(0.f, 20.f);
			if( ImGui::Button(u8"\uE810 다시시작") ) {
				setNewGame();
			}
		}
		ImGui::PopFont();
		ImGui::End();
	}



	//
	//	Settings
	//
	static const char* levelStrs[4] = {u8"초급", u8"중급", u8"고급", u8"커스텀"};
	static int level = 0;
	static bool isAdjChanged = false;
	if( game_state == GameState::NEW ) {
		ImGui::Begin("settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::PushFont(g_ui_font);
		ImGui::SetNextItemWidth(100.f);
		if( ImGui::Combo("level", &level, levelStrs, 4) ) {
			isAdjChanged = true;
			switch( level ) {
			case 0:
				adj.width  = 9;
				adj.height = 9;
				adj.nr_mine= 9;
				break;
			case 1:
				adj.width  = 16;
				adj.height = 16;
				adj.nr_mine= 40;
				break;
			case 2:
				adj.width  = 30;
				adj.height = 16;
				adj.nr_mine= 99;
				break;
			}
			const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			int cellWinSize = NINE_BOARD_WINDOW_SIZE/9;
			int newWidth = glm::min( cellWinSize*adj.width, vidMode->width );
			int newHeight = glm::min( cellWinSize*adj.height+TOP_WINDOW_HEIGHT, vidMode->width );
			glfwSetWindowSize(window, newWidth, newHeight);
			glfwSetWindowPos(window, (vidMode->width-newWidth)/2, (vidMode->height-newHeight)/2);
		}
		ImGui::SameLine(0.f, 20.f);
		ImGui::Text("%d in ( %d x %d )", adj.nr_mine, adj.width, adj.height);

		static const float minMineRatio = 0.12f;
		static const float maxMineRatio = 0.25f;
		if( level==3 ) {
			const float sliderWidth = (ImGui::GetContentRegionAvail().x)/3.f*0.7f;
			ImGui::PushItemWidth(sliderWidth);
			isAdjChanged |= ImGui::SliderInt("width", &adj.width, 9, MAX_BOARD_SIZE);
			ImGui::SameLine(0.f, 15.f);
			isAdjChanged |= ImGui::SliderInt("height", &adj.height, 9, MAX_BOARD_SIZE);
			ImGui::SameLine(0.f, 15.f);
			int nrCells = adj.width*adj.height;
			if( isAdjChanged ) {
				adj.nr_mine = glm::clamp(adj.nr_mine, int(minMineRatio*nrCells), int(maxMineRatio*nrCells));
			}
			isAdjChanged |= ImGui::SliderInt("mine", &adj.nr_mine, int(minMineRatio*nrCells), int(maxMineRatio*nrCells));
			ImGui::PopItemWidth();
		}
		if( isAdjChanged ) {
			board.width = adj.width;
			board.height = adj.height;
			board.nr_mine = adj.nr_mine;
		}
		ImGui::PopFont();
		ImGui::End();
	}
	

	//
	//	Input
	//
	if( game_state==GameState::OVER || !isBoardHovered || board.isOutside(hoveredX, hoveredY)) {
		return;
	}
	if( ImGui::IsMouseReleased(0) ) {
		if( game_state == GameState::NEW ) {
			game_state = GameState::PLAYING;
			start_time = ImGui::GetTime();
			board.plantMines(hoveredX, hoveredY);
		}
		if( board.reveal(hoveredX, hoveredY) ) {
			game_state = GameState::OVER;
		}
		log::pure("isOver: %d %d\n", hoveredX, hoveredY);
	} else if( ImGui::IsMouseReleased(1) ) {
		board.switchFlag(hoveredX, hoveredY);
	}
}