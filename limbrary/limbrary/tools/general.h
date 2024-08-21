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
	constexpr int SPRINTF_BUF_SIZE = 128;

	template<typename T>
	inline void safeDel(T*& p) {
		delete p; // delete nullptr is ok
		p = nullptr;
	}

	inline const char* boolStr(bool b) {
		return b ? "true" : "false";
	}
	inline const char* boolStrOX(bool b) {
		return b ? "O" : "X";
	}

	// From: https://en.cppreference.com/w/cpp/algorithm/random_shuffle
	// removed std::random_shuffle in cpp 17
	template<typename T>
	void randomShuffle(std::vector<T>& vec) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(vec.begin(), vec.end(), g);
	}

	std::string readStrFromFile(std::string_view path);
	void writeStrToFile(std::string_view path, std::string_view text);

	char* fmtStrToBuf(const char* format, ...);

	template <typename T>
	inline int findIdx(const std::vector<T>& v, const T& value) {
		auto it = find(v.begin(), v.end(), value);
		if (it != v.end()) {
			return (int)std::distance(v.begin(), it);
		}
		return -1;
	}

	template <typename T>
	inline typename std::vector<T>::iterator findIdxIt(const std::vector<T>& v, const T& value) {
		return find(v.begin(), v.end(), value);
	}

	inline std::string strTolower( std::string_view str ) {
		std::string a(str);
		std::transform( a.begin(), a.end(), a.begin(), [](auto c) { return std::tolower(c); });
		return a;
	}

	inline bool strIsSame( std::string_view a, std::string_view b ) {
		return strTolower(a)==strTolower(b);
	}

	inline std::string getExtension( const std::string& filename ) {
		size_t dotPos = filename.find_last_of('.');
		if( dotPos == std::string::npos || dotPos == filename.length()-1 )
			return "";
		return filename.substr(dotPos+1);
	}

	inline std::string getName( const std::string& filename ) {
		size_t dotPos = filename.find_last_of('.');
		if( dotPos == std::string::npos )
			return filename;
		return filename.substr(0,dotPos);
	}

	template <typename T>
	inline bool isIn( const std::vector<T>& arr, const T& value ) {
		return arr.find(value) != arr.end();
	}
	
	template <typename TK, typename TV>
	inline bool isIn( const std::map<TK, TV>& map, const TK& key ) {
		return map.find(key) != map.end();
	}
}
#endif