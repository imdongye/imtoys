#pragma once

#define COMP_IMVEC2(X, Y) ((X).x==(Y).x)&&((X).y==(Y).y)

namespace lim
{
	constexpr float Q_PI = 0.78539816339f; // quarter
	constexpr float H_PI = 1.57079632679f; // half
	constexpr float F_PI = 3.14159265359f; // float
	constexpr float D_PI = 6.28318530718f; // double

	//using Callbacks = std::map<void* const, std::function<_Fty>>;
	template <class _Fty>
	struct Callbacks: public std::map<void* const, std::function<_Fty>>
	{
	};
}