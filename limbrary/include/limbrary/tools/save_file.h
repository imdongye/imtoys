/*
	imdongye@naver.com
	fst: 2022-09-18
	lst: 2024-11-19
*/

#ifndef __save_file_h_
#define __save_file_h_

#include "mecro.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace lim
{
	namespace save_file
	{
		extern nlohmann::json data;

		void init();
		void deinit();
		void saveToFile();
	}
}

#endif
