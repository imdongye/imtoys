/*
	general tools
	2024-08-07 / im dongye
*/
#ifndef __g_tools_h_
#define __g_tools_h_

#include <string>
#include <glad/glad.h>
#include "log.h"
#include <algorithm>
#include <map>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define COMP_IMVEC2(X, Y) ((X).x==(Y).x)&&((X).y==(Y).y)
#define INT2VOIDP(i) (void*)(uintptr_t)(i)

namespace lim
{
	template<typename T>
	inline void safeDel(T*& p) {
		delete p; // delete nullptr is ok
		p = nullptr;
	}

	// From: https://en.cppreference.com/w/cpp/algorithm/random_shuffle
	// removed std::random_shuffle in cpp 17
	template<typename T>
	void randomShuffle(std::vector<T>& vec) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(vec.begin(), vec.end(), g);
	}

	template <typename T>
	inline bool isIn( const std::vector<T>& arr, const T& value ) {
		return arr.find(value) != arr.end();
	}
	
	template <typename TK, typename TV>
	inline bool isIn( const std::map<TK, TV>& map, const TK& key ) {
		return map.find(key) != map.end();
	}

	template <typename T>
	inline typename std::vector<T>::iterator findIdxIt(const std::vector<T>& v, const T& value) {
		return find(v.begin(), v.end(), value);
	}

	template <typename T>
	inline int findIdx(const std::vector<T>& v, const T& value) {
		auto it = find(v.begin(), v.end(), value);
		if (it != v.end()) {
			return (int)std::distance(v.begin(), it);
		}
		return -1;
	}
}

//
//	text tools
//
namespace lim
{

	std::string readStrFromFile(const char* path);
	
	void writeStrToFile(const char* path, const char* text);
	
	constexpr int SPRINTF_BUF_SIZE = 128;
	const char* fmtStrToBuf(const char* format, ...);

	

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