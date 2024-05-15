//
//  minesweeper
//	2024-05-14 / im dong ye
//

#ifndef __app_mines_weeper_h_
#define __app_mines_weeper_h_

#include <limbrary/application.h>

namespace lim
{
	class AppMineSweeper : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "minesweeper demo";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DESCRIPTION = "...";
	public:
		AppMineSweeper();
		~AppMineSweeper();
		virtual void update() override;
		virtual void updateImGui() override;
		// virtual void keyCallback(int key, int scancode, int action, int mods) override;
		// virtual void cursorPosCallback(double xPos, double yPos) override;

	private:
		static constexpr int MAX_BOARD_SIZE = 40;
		
		enum class GameState{
			NEW,
			PLAYING,
			OVER,
		};
		struct Adjustable { // for ui
			int width = 9;
			int height = 9;
			int nr_mine = 10;
		};
		struct Cell {
			bool is_open, is_mine, is_flaged;
			int nr_nbrs;
		};
		struct Board {
			Cell cells[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
			int width, height;
			int nr_closed, nr_flagged, nr_mine;

			bool isOutside(int x, int y);
			void clear();
			void plantMines(int avoidX, int avoidY);
			bool reveal(int x, int y);
			void switchFlag(int x, int y);
		};

		Adjustable adj;
		double start_time;
		double elapsed_time;
		GameState game_state;
		Board board;

	private:
		void setNewGame();
	};
}

#endif
