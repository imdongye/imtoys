/*
객체의 포인터를 키로한 callbacks를 map대신 unordered_map을 사용하는 이유:
callbacks은 매 프레임 호출될수도 있기 때문에 메모리를 많이 써도 순회 하는 속도는 빨라야한다.
unordered_map은 헤쉬테이블로 메모리를 많이 차지 하지만 순회는 상수시간복잡도이다.
반면에 map은 tree구조로 탐색과 동시에 생성하는 작업이 성능에 영향이 없어서 지원해준다. 
그리고 메모리도 추가로 필요하지 않아서 효율적이다. 하지만 트리구조라 순회할때 logn의 시간복잡도라 여기에서 사용할수없다.
최종적으로 unordered_map은 상수시간복잡도이지만 상수시간이 커서 실시간에서 순회접근은 어렵다. 그래서 키와 콜백을 vector로 관리하는 템플릿 클래스를 생성함.
*/
#ifndef __utils_h_
#define __utils_h_

#include <string>
#include <functional>
#include <vector>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <algorithm>

#define COMP_IMVEC2(X, Y) ((X).x==(Y).x)&&((X).y==(Y).y)
#define INT2VOIDP(i) (void*)(uintptr_t)(i)

namespace lim
{
	constexpr int SPRINTF_BUF_SIZE = 128;
	constexpr float Q_PI = 0.78539816339f; // quarter
	constexpr float H_PI = 1.57079632679f; // half
	constexpr float F_PI = 3.14159265359f; // float
	constexpr float D_PI = 6.28318530718f; // double

	using CStr = const char* const;

	std::string readStrFromFile(std::string_view path);
	void writeStrToFile(std::string_view path, std::string_view text);

	char* fmtStrToBuf(const char* format, ...);

	template <typename T>
	inline int findIdx(const std::vector<T>& v, const T& value) {
		auto it = find(v.begin(), v.end(), value);
		if (it != v.end()) {
			return std::distance(v.begin(), it);
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