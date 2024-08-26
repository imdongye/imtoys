/*
    2024-08-27 / imdongye

*/
#ifndef __mecro_h_
#define __mecro_h_


#define NO_COPY_AND_MOVE(ClassName) \
ClassName(const ClassName& src) = delete; \
ClassName& operator=(const ClassName& src) = delete; \
ClassName(ClassName&& src) = delete; \
ClassName& operator=(ClassName&& src) = delete;


namespace lim
{
    template<typename T>
    struct NoCopyMove {
        T(const T&) = delete;
        T& operator=(const T&) = delete;
        T(T&&) = delete;
        T& operator=(T&&) = delete;
    };
}


#endif