/*
	Todo:
	1. 맵정보 내보내기 불러오기
	2. 나무위키 규칙보고 compute업그레이드
	3. high dip From: https://github.com/ocornut/imgui/issues/1065

*/
#include "app_mine_sweeper.h"
#include <glad/glad.h>
#include <limbrary/tools/log.h>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <functional>
#include <set>
#include "locale.h"


using namespace glm;
using namespace lim;

namespace {
	constexpr int MAX_BOARD_SIZE = 40;
	constexpr int TOP_WINDOW_HEIGHT = 100;
	constexpr int NINE_BOARD_WINDOW_SIZE = 380;
	constexpr float MINI_FONT_SIZE = 16.f;
	constexpr float UI_FONT_SIZE = 20.f;
	constexpr float CELL_FONT_SIZE = 30.f;
		
	enum UiState : int {
		US_NEW,
		US_PLAYING,
		US_OVER,
	};
	struct Adjustable { // for ui
		int width = 9;
		int height = 9;
		int nr_mine = 10;
	};
	struct Cell {
		int x, y;
		bool is_open, is_mine, is_flaged;
		int nr_nbrs;
		float prob; // probability of is mine
	};
	struct Board {
		Cell cells[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
		int width, height;
		int nr_closed, nr_flagged, nr_mine;
		int nr_remain_view; // 실제 남은 개수가 아니라 추측되는 개수

		bool isOutside(int x, int y);
		void clear();
		void plantMines(int avoidX, int avoidY);
		void dig(int x, int y);
		void switchFlag(int x, int y);

		void compute1();
		void compute2();
		void invokeNbrs(int x, int y, std::function<void(Cell&)> invoke);
		void invokeAll(std::function<void(Cell&)> invoke);
	};
		

	vec2 toGlm(const ImVec2& v) {
		return {v.x, v.y};
	}
	ImVec2 toIg(const vec2& v) {
		return {v.x, v.y};
	}
	
	ImFont* ui_font;
	ImFont* cell_font;
	Adjustable adj;
	double start_time;
	double elapsed_time;
	UiState game_state;
	Board board;
	bool is_game_win = false;
}




bool Board::isOutside(int x, int y) {
	return ( x<0 || x>=width || y<0 || y>=height );
}
void Board::clear() {
	for( int y=0; y<MAX_BOARD_SIZE; y++ ) for( int x=0; x<MAX_BOARD_SIZE; x++ ) {
		Cell& c = cells[y][x];
		c.x = x; c.y = y;
		c.is_open = false;
		c.is_mine = false;
		c.is_flaged = false;
		c.nr_nbrs = 0;
	}
}
void Board::invokeNbrs(int x, int y, std::function<void(Cell&)> invoke) {
	for( int dy=-1; dy<=1; dy++ ) for( int dx=-1; dx<=1; dx++ ) {
		if( dy==0 && dx==0 )
			continue;
		int nx = x+dx;
		int ny = y+dy;
		if( isOutside(nx, ny) )
			continue;
		invoke(cells[ny][nx]);
	}
}
void Board::invokeAll(std::function<void(Cell&)> invoke) {
	for( int y=0; y<height; y++ ) for( int x=0; x<width; x++ ) {
		invoke(cells[y][x]);
	}
}
void Board::plantMines(int avoidX, int avoidY) {
	nr_closed = width*height;
	nr_remain_view = nr_mine;
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
		cells[y][x].nr_nbrs = 0;
		invokeNbrs(x,y,[](Cell& c){
			if(!c.is_mine)
				c.nr_nbrs++;
		});
	}
}
void Board::switchFlag(int x, int y) {
	if( isOutside(x, y) )
		return;
	Cell& cell = cells[y][x];
	if( cell.is_open )
		return;
	cell.is_flaged = !cell.is_flaged;
	nr_flagged += cell.is_flaged ? 1 : -1;
	nr_remain_view = glm::min(board.nr_closed, board.nr_mine-board.nr_flagged);
}
void Board::dig(int x, int y) {
	Cell& cell = cells[y][x];
	if( isOutside(x, y) || cell.is_open )
		return;
	if( cell.is_flaged ) 
		switchFlag(x,y);
	cell.is_open = true;
	nr_closed--;
	nr_remain_view = glm::min(board.nr_closed, board.nr_mine-board.nr_flagged);

	if( cell.nr_nbrs==0 && !cell.is_mine ) {
		invokeNbrs(x,y,[&](Cell& c){
			dig(c.x, c.y);
		});
	}
	if( cell.is_mine ) {
		nr_remain_view = 1;
		invokeAll([&](Cell& c){
			if( !c.is_open && !c.is_flaged ) {
				nr_remain_view++;
			}
		});
		game_state = US_OVER;
		is_game_win = false;
	}
	else if( nr_closed==nr_mine ) {
		game_state = US_OVER;
		is_game_win = true;
	}
}

//
//	검증
//
/*
	열린칸의 지뢰개수를 보고 지뢰 찾기
	vs 모르는칸의 주변칸 지뢰개수표시 보고 지뢰인지 확인

	열린칸은 flag일 수 없다고 가정

	compute1: 열린칸 지뢰개수 보고 나머지 전부 지뢰 또는 빈칸
	compute2: 열린칸에서 주변 모르는칸에 확률(남은지뢰개수/모르는칸개수)을 더해서 높은것 지뢰로 선택
	compute3: Todo: 확률로 칸열기
*/
void Board::compute1() {
	invokeAll([&](Cell& c) {
		if( !c.is_open ) {
			return;
		}
		Cell* unknownNbrs[8];
		int nrFlags = 0;
		int nrUnknowns = 0;
		invokeNbrs(c.x, c.y, [&](Cell& nbr){
			if( nbr.is_flaged )
				nrFlags++;
			else if( !nbr.is_open ) 
				unknownNbrs[nrUnknowns++] = &nbr;
		});
		if( c.nr_nbrs == nrFlags ) {
			for( int i=0; i<nrUnknowns; i++ ) {
				dig(unknownNbrs[i]->x, unknownNbrs[i]->y);
			}
		}
		if( c.nr_nbrs-nrFlags == nrUnknowns ) {
			for( int i=0; i<nrUnknowns; i++ ) {
				switchFlag(unknownNbrs[i]->x, unknownNbrs[i]->y);
			}
		}
	});
}
void Board::compute2() {
	Cell *maxCell, *minCell;
	std::set<Cell*> unknowns;

	invokeAll([&](Cell& c) {
		c.prob = 0.f;
	});

	invokeAll([&](Cell& c) {
		if( !c.is_open ) {
			return;
		}
		Cell* unknownNbrs[8];
		int nrFlags = 0;
		int nrUnknownNbrs = 0;
		invokeNbrs(c.x, c.y, [&](Cell& nbr){
			if( nbr.is_flaged )
				nrFlags++;
			else if( !nbr.is_open ) 
				unknownNbrs[nrUnknownNbrs++] = &nbr;
		});
		for( int i=0; i<nrUnknownNbrs; i++ ) {
			unknownNbrs[i]->prob += (c.nr_nbrs-nrFlags) / float(nrUnknownNbrs);
			unknowns.insert(unknownNbrs[i]);
		}
	});
	
	maxCell = minCell = *unknowns.begin();
	
	for( Cell* curCell : unknowns ) {
		log::pure("%f\n", curCell->prob);
		if( maxCell->prob < curCell->prob ) {
			maxCell = curCell;
		} 
		else if( minCell->prob > curCell->prob ) {
			minCell = curCell;
		}
	}
	log::pure("max: %f\n", maxCell->prob);
	log::pure("min: %f\n\n", minCell->prob);

	const float averageMedian = 0.955f; // 테스트로 얻은 표본에서의 중간값 평균
	if( maxCell->prob - averageMedian > averageMedian - minCell->prob ) {
		switchFlag(maxCell->x, maxCell->y);
	}
	else {
		dig(minCell->x, minCell->y);
	}

}







static void setNewGame() {
	game_state = US_NEW;
	elapsed_time = 0.0;
	board.clear();
	board.width = adj.width;
	board.height = adj.height;
	board.nr_mine = adj.nr_mine;
}

AppMineSweeper::AppMineSweeper() 
	: AppBase(NINE_BOARD_WINDOW_SIZE, NINE_BOARD_WINDOW_SIZE+TOP_WINDOW_HEIGHT, APP_NAME)
{
	lang::set(lang::LC_KOR);
	glfwSetWindowTitle(window, lang::title);

	ImGuiIO& io = ImGui::GetIO();
	const char* fontPath = "assets/fonts/SpoqaHanSansNeo-Medium.ttf";
	const char* iconPath = "im_mine_sweeper/fontello/font/fontello.ttf";
	io.Fonts->AddFontFromFileTTF(fontPath, MINI_FONT_SIZE, nullptr, io.Fonts->GetGlyphRangesKorean());
	
	ImFontConfig config;
    // config.GlyphMinAdvanceX = 13.0f;
	config.MergeMode = true;
	static ImWchar lIconRanges[] = { 0xE800, 0xE810, 0 };

	io.Fonts->AddFontFromFileTTF(fontPath, UI_FONT_SIZE, nullptr, io.Fonts->GetGlyphRangesKorean());
	ui_font = io.Fonts->AddFontFromFileTTF(iconPath, UI_FONT_SIZE, &config, lIconRanges);

	io.Fonts->AddFontFromFileTTF(fontPath, CELL_FONT_SIZE);
	cell_font = io.Fonts->AddFontFromFileTTF(iconPath, CELL_FONT_SIZE, &config, lIconRanges);

	setNewGame();
}
AppMineSweeper::~AppMineSweeper()
{
    log::exportToFile();
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
#ifdef LIM_DEBUG 
	log::drawViewer();
#endif
	//
	//	Board
	//
	static const int COL_CELL_IDX = 0;
	static const int COL_MINE_IDX = 9;
	static const int COL_FLAG_RIGHT_IDX = 10;
	static const int COL_FLAG_WRONG_IDX = 11;
	static const ImColor boardColors[12] = {
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
	ImGui::SetNextWindowSize({NINE_BOARD_WINDOW_SIZE, NINE_BOARD_WINDOW_SIZE}, ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{4, 4});
	ImGui::Begin(lang::board);
	const bool isBoardHovered = ImGui::IsWindowHovered();
	const float paddingRadio = 0.06f;
	const vec2 contentSize = toGlm(ImGui::GetContentRegionAvail());

	const vec2 cellRegion = contentSize/vec2(board.width, board.height);
	const vec2 cellSize = cellRegion*vec2(1.f-paddingRadio);
	const vec2 cellPadding = cellRegion*vec2(paddingRadio/2.f);
	const float rounding = cellSize.x * 0.1f;
	const float baseFontSize = glm::min(glm::min(cellSize.x*0.9f, cellSize.y)*0.8f, CELL_FONT_SIZE);


	const vec2 screenPos = toGlm(ImGui::GetCursorScreenPos());
	vec2 mousePos = toGlm(ImGui::GetMousePos()) - screenPos;
	int hoveredX = (mousePos.x>0)? int(mousePos.x/cellRegion.x) : -1;
	int hoveredY = (mousePos.y>0)? int(mousePos.y/cellRegion.y) : -1;
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	board.invokeAll([&](Cell& cell) {
		bool isHovered = hoveredX==cell.x && hoveredY==cell.y;
		ImVec2 tlPos = toIg(screenPos + cellPadding + vec2{cell.x, cell.y}*cellRegion);
		ImVec2 brPos = {tlPos.x+cellSize.x, tlPos.y+cellSize.y};
		ImColor col = boardColors[COL_CELL_IDX];

		if( cell.is_open ) {
			drawList->AddRect(tlPos, brPos, col, rounding);
			if( !cell.is_mine && cell.nr_nbrs==0 ) {
				return;
			}
		}
		else { // closed
			if( isHovered || (game_state == US_OVER && cell.is_mine) ) {
				col.Value.w = 0.3f;
			}
			drawList->AddRectFilled(tlPos, brPos, col, rounding);
			if( game_state == US_OVER ) {
				if( !cell.is_flaged && !cell.is_mine)
					return;
			}
			else {
				if( !cell.is_flaged )
					return;
			}
		}

		const char* txPtr;
		ImVec2 txPos;
		float fontSize = baseFontSize;
		txPos.x = tlPos.x+cellSize.x*0.5f;
		txPos.y = tlPos.y+cellSize.y*0.5f;

		if( cell.is_flaged ) {
			fontSize *= 0.8f;
			txPos.x -= fontSize*0.21f;
			col = boardColors[(game_state==US_OVER&&cell.is_mine)?COL_FLAG_RIGHT_IDX:COL_FLAG_WRONG_IDX];
			txPtr = u8"\uE800";
		} 
		else if( cell.is_mine ) {
			if( game_state == US_PLAYING && !cell.is_open )
				return;
			fontSize *= 0.8f;
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
		txPos.x -= fontSize*0.23f;
		txPos.y -= fontSize*0.45f;
		drawList->AddText(cell_font, fontSize, txPos, col, txPtr);
	}); // end invokeAll
	ImGui::End();
	ImGui::PopStyleVar();

	//
	//	Board Input
	//
	if( game_state!=US_OVER && isBoardHovered && !board.isOutside(hoveredX, hoveredY)) {
		if( ImGui::IsMouseReleased(0) ) {
			if( game_state == US_NEW ) {
				game_state = US_PLAYING;
				start_time = ImGui::GetTime();
				board.plantMines(hoveredX, hoveredY);
			}
			board.dig(hoveredX, hoveredY);
		} else if( ImGui::IsMouseReleased(1) && game_state==US_PLAYING ) {
			board.switchFlag(hoveredX, hoveredY);
		}
	}
	



	//
	//	State
	//
	if( game_state != US_NEW ) {
		ImGui::Begin(lang::state, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushFont(ui_font);

		if( game_state == US_PLAYING ) {
			elapsed_time = ImGui::GetTime() - start_time;
		}
		int eSec = (int)elapsed_time;
		ImGui::AlignTextToFramePadding();
		if( game_state==US_OVER ) {
			ImGui::TextUnformatted(( is_game_win )?lang::clear:lang::over);
			ImGui::SameLine(0.f,20.f);
		}
		ImGui::Text("\uE803 %d:%02d", eSec/60, eSec%60);
		ImGui::SetItemTooltip(lang::sec_count, elapsed_time);

		ImGui::SameLine();
		ImGui::Text("\uE802 %2d/%d", board.nr_remain_view, board.nr_mine);
		if( game_state != US_NEW ) {
			if( game_state == US_PLAYING ) {
				ImGui::SameLine();
				if( ImGui::Button(lang::auto_btn1) ) {
					board.compute1();
				}
				if( ImGui::BeginItemTooltip() ) {
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
					ImGui::TextUnformatted(lang::auto_btn1_disc);
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
				ImGui::SameLine();
				if( ImGui::Button(lang::auto_btn2) ) {
					board.compute2();
				}
				if( ImGui::BeginItemTooltip() ) {
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
					ImGui::TextUnformatted(lang::auto_btn2_disc);
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			} 
			ImGui::SameLine();
			if (ImGui::Button(lang::retry)) {
				setNewGame();
			}
		}
		ImGui::PopFont();
		ImGui::End();
	}



	//
	//	Settings
	//
	static int level = 0;
	static bool isAdjChanged = false;
	if( game_state == US_NEW ) {
		ImGui::Begin(lang::setting, nullptr, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushFont(ui_font);
		ImGui::SetNextItemWidth(100.f);
		if( ImGui::Combo(lang::level, &level, lang::levels, 4) ) {
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
		ImGui::SetItemTooltip("%d%%", int(adj.nr_mine/float(adj.width*adj.height)*100));

		ImGui::SameLine(0.f, 20.f);
		ImGui::TextDisabled("(?)");
		if( ImGui::BeginPopupContextItem("context") ) {
			static int cur_locale = 0;
			ImGui::PushItemWidth(100.f);
			if(ImGui::Combo(lang::locale, &cur_locale, lang::locale_strs)) {
				lang::set((lang::LOCALE)cur_locale);
			}
			ImGui::PopItemWidth();
            ImGui::EndPopup();
		} else if( ImGui::BeginItemTooltip() ) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 15.0f);
			ImGui::TextUnformatted(lang::game_disc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		if(level>1) {
			ImGui::SameLine(0.f, 20.f);
			ImGui::TextDisabled(lang::thanks);
			ImGui::SetItemTooltip(lang::programmer);
		}

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
}
