#ifndef __utils_h_
#define __utils_h_

#include <string>
#include <functional>
#include <map>

#define COMP_IMVEC2(X, Y) ((X).x==(Y).x)&&((X).y==(Y).y)
#define INT2VOIDP(i) (void*)(uintptr_t)(i)

namespace lim
{
	constexpr int SPRINTF_BUF_SIZE = 128;
	constexpr float Q_PI = 0.78539816339f; // quarter
	constexpr float H_PI = 1.57079632679f; // half
	constexpr float F_PI = 3.14159265359f; // float
	constexpr float D_PI = 6.28318530718f; // double

	template <class _Fty>
	using Callbacks = std::map<void* const, std::function<_Fty>>;

	using CStr = const char* const;

	std::string getStrFromFile(std::string_view path);
	void setStrToFile(std::string_view path, std::string_view text);

	char* fmtStrToBuf(const char* format, ...);
}
#endif