/*
객체의 포인터를 키로한 callbacks를 map대신 unordered_map을 사용하는 이유:
callbacks은 매 프레임 호출될수도 있기 때문에 메모리를 많이 써도 순회 하는 속도는 빨라야한다.
unordered_map은 헤쉬테이블로 메모리를 많이 차지 하지만 순회는 상수시간복잡도이다.
하지만 없는 키를 []로 접근했을때 생성해주지 않고 null을 반환한다. 따로 insert(emplace)로 생성하고 접근해야한다
반면에 map은 tree구조로 탐색과 동시에 생성하는 작업이 성능에 영향이 없어서 지원해준다. 
그리고 메모리도 추가로 필요하지 않아서 효율적이다. 하지만 트리구조라 순회할때 logn의 시간복잡도라 여기에서 사용할수없다.
*/
#ifndef __utils_h_
#define __utils_h_

#include <string>
#include <functional>
#include <vector>
#include <glad/glad.h>
#include <limbrary/log.h>

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
	int findIdx(const std::vector<T>& v, const T& value) {
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
	// 객체 배열에서 포인터로 인덱스를 찾는다.
	// Todo: 포인터 산술연산 테스트
	// Todo: function에 객체 포인터 있을것같은데
	template <typename T>
	GLuint findPtIdxInObjArr(const std::vector<T>& src, const T* target)
	{
		int idx=0;
		for( ; idx<src.size(); idx++ ) {
			if( &src[idx] == target ) {
				return idx;
			}
		}
		return -1;
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
				log::err("same key in callbacks\n\n");
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