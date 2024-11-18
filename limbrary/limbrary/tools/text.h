/*
	imdongye@naver.com
	fst: 2024-08-07
	lst: 2024-08-07
*/
#ifndef __tools_text_h_
#define __tools_text_h_

#include <string>
#include <algorithm>


//
//	text tools
//
namespace lim
{
	constexpr int FMT_STR_BUF_SIZE = 512;
	const char* fmtStrToBuf(const char* format, ...);

	std::string readStrFromFile(const char* path);
	
	void writeStrToFile(const char* path, const char* text);

	

	inline std::string strTolower(const char* str) {
		std::string s(str);
		std::transform( s.begin(), s.end(), s.begin(), [](auto c) { return std::tolower(c); });
		return s;
	}
	inline bool strIsSame( const char* a, const char* b ) {
		return strTolower(a)==strTolower(b);
	}

	inline const char* getExtension( const char* path ) {
		return strrchr(path, '.')+1;
	}
	inline const char* getFileName(const char* path) {
		const char* lastSlash = strrchr(path, '/');
		const char* lastSlash2 = strrchr(path, '\\');
		if( lastSlash<lastSlash2 ) {
			lastSlash = lastSlash2;
		}
		return lastSlash ? lastSlash+1 : path;
	}
	inline std::string getDirectory(const std::string& path) {
		const size_t lastSlashPos = path.find_last_of("/\\");
		return (lastSlashPos>0) ? path.substr(0, lastSlashPos+1) : "";
	}
	inline std::string getFileNameWithoutExt(const char* path) {
		std::string name = getFileName(path);
		name = name.substr(0, name.find_last_of('.'));
		return name;
	}

	inline const char* boolStr(bool b) {
		return b ? "true" : "false";
	}
	inline const char* boolOX(bool b) {
		return b ? "O" : "X";
	}
}


#endif