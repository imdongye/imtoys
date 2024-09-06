/*

	2022-09-18 / im dong ye

	global save file data

Note:
	Singleton

*/

#ifndef __s_save_file_h_
#define __s_save_file_h_

#include "mecro.h"
#include <string>
#include <vector>
#include <nlohmann/json.h>

namespace lim
{
	class SaveFile : public SingletonDynamic<SaveFile>
	{
	private:
		std::string file_path = "app_dir/app_save.json";
		mutable nlohmann::json jfile;

	public:
		nlohmann::json data;

	private:
		friend SingletonDynamic<SaveFile>;
		SaveFile();
		~SaveFile();

	public:
		void saveToFile() const;
	};
}

#endif
