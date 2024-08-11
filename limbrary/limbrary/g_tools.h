/*
	global tools
	2024-08-07 / im dongye


Note: callbacks
	객체의 포인터를 키로한 callbacks를 map대신 unordered_map을 사용하는 이유:
	callbacks은 매 프레임 호출될수도 있기 때문에 메모리를 많이 써도 순회 하는 속도는 빨라야한다.
	unordered_map은 헤쉬테이블로 메모리를 많이 차지 하지만 순회는 상수시간복잡도이다.
	반면에 map은 tree구조로 탐색과 동시에 생성하는 작업이 성능에 영향이 없어서 지원해준다. 
	그리고 메모리도 추가로 필요하지 않아서 효율적이다. 하지만 트리구조라 순회할때 logn의 시간복잡도라 여기에서 사용할수없다.
	최종적으로 unordered_map은 상수시간복잡도이지만 상수시간이 커서 실시간에서 순회접근은 어렵다. 그래서 키와 콜백을 vector로 관리하는 템플릿 클래스를 생성함.
*/
#ifndef __g_tools_h_
#define __g_tools_h_

#include <string>
#include <functional>
#include <vector>
#include <glad/glad.h>
#include <limbrary/log.h>
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

	using CStr = const char* const;

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


	namespace log {
		inline void pure(const glm::vec2& v) {
			log::pure("%-3.3f %-3.3f\n", v.x, v.y);
		}
		inline void pure(const glm::vec3& v) {
			log::pure("%-3.3f %-3.3f %-3.3f\n", v.x, v.y, v.z);
		}
		inline void pure(const glm::vec4& v) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", v.x, v.y, v.z, v.w);
		}
		inline void pure(const glm::quat& q) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", q.x, q.y, q.z, q.w);
		}
		inline void pure(const glm::mat3& m) {
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[0][0], m[0][1], m[0][2]);
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[1][0], m[1][1], m[1][2]);
			log::pure("%-3.3f %-3.3f %-3.3f\n", m[2][0], m[2][1], m[2][2]);
		}
		inline void pure(const glm::mat4& m) {
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
			log::pure("%-3.3f %-3.3f %-3.3f %-3.3f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
		}
	}

	// From: https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
	inline int fastIntCeil(int x, int y) {
		return (x % y) ? x / y + 1 : x / y;
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


	

	//template <class Ftype>
	//using Callbacks = std::map<const void*, std::function<Ftype>>;
	template <typename Ftype>
	class Callbacks {
	public:
		std::vector<const void*> keys;
		std::vector<std::function<Ftype>> cbs;
	private:
		Callbacks(const Callbacks&) = delete;
		Callbacks& operator=(const Callbacks&) = delete;
	public:
		Callbacks(){}
		Callbacks(Callbacks&& src) noexcept
		{
			*this = std::move(src);
		}
		Callbacks& operator=(Callbacks&& src) noexcept
		{
			if(this == &src)
				return *this;
			keys = std::move(keys);
			cbs = std::move(cbs);
			return *this;
		}
		~Callbacks() noexcept
		{
		}
		typename std::function<Ftype>& operator[](const void* key)
		{
			int idx = 0;
			for( const void* k : keys ) {
				if( k == key )
					break;
				idx++;
			}
			if( idx < keys.size() )
				return cbs[idx];
			keys.push_back(key);
			cbs.push_back(std::function<Ftype>());
			return cbs.back();
		}
		bool isIn(void* key) {
			for( const void* k : keys ) {
				if( k == key )
					return true;
			}
			return false;
		}
		void erase(void* key) 
		{
			int idx = 0;
			for( const void* k : keys ) {
				if( k == key )
					break;
				idx++;
			}
			if( idx >= keys.size() ) {
				log::err("no key in callbacks\n\n");
				return;
			}
			keys.erase(keys.begin()+idx);
			cbs.erase(cbs.begin()+idx);
		}
		void changeKey(const void* src, const void* dst)
		{
			if( src == dst ) {
				log::err("same key input in callbacks\n\n");
				return;
			}
			for( auto& key : keys ) {
				if( key == src ) {
					key = dst;
					return;
				}
			}
			log::err("no key in callbacks\n\n");
		}
		typename std::vector<std::function<Ftype>>::iterator begin()
		{
			return cbs.begin();
		}
		typename std::vector<std::function<Ftype>>::iterator end()
		{
			return cbs.end();
		}
	};
}
#endif