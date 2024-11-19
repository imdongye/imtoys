/*
	imdongye@naver.com
	fst: 2024-08-07
	lst: 2024-08-07
*/

#ifndef __g_tools_h_
#define __g_tools_h_

#include <vector>
#include <map>
#include <random>

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

#endif