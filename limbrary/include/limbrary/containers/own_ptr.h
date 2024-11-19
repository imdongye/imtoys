/*
    imdongye@naver.com
	fst: 2024-08-24
	lst: 2024-08-24

Note:
    OwnPtr is a my smart pointer
    when copy then object copied
    when delete then object deleted
    when move then raw pointer moved
    and nullable
*/
#ifndef __own_ptr_h_
#define __own_ptr_h_

#include <vector>

namespace lim
{
    template<typename T>
    struct OwnPtr
    {
        T* raw;

        OwnPtr() : raw(nullptr) {}
        OwnPtr(T* ptr) : raw(ptr) {}
        OwnPtr(const OwnPtr<T>& src) : raw( src.raw ? new T(*src.raw) : nullptr ) {}
        OwnPtr<T>& operator=(const OwnPtr<T>& src) { delete raw; raw = src.raw ? new T(*src.raw) : nullptr; return *this; }
        OwnPtr<T>& operator=(T* ptr) { delete raw; raw = ptr; return *this; }
        OwnPtr(OwnPtr<T>&& src) noexcept : raw(src.raw) { src.raw = nullptr; }
        OwnPtr<T>& operator=(OwnPtr<T>&& src) noexcept { delete raw; raw = src.raw; src.raw = nullptr; return *this; }
        ~OwnPtr() noexcept { delete raw; }

        void clear() noexcept { delete raw; raw = nullptr; }

        T* operator->() const noexcept { return raw;  }
        T& operator* () const noexcept { return *raw; }
        operator bool() const noexcept { return raw != nullptr; }
        bool operator==(const OwnPtr<T>& src) const noexcept { return raw == src.raw; }
        bool operator!=(const OwnPtr<T>& src) const noexcept { return raw != src.raw; }
        bool operator==(const T* ptr) const noexcept { return raw == ptr; }
        bool operator!=(const T* ptr) const noexcept { return raw != ptr; }
    };

    // template <typename T>
    // inline T* operator=(T* dst, const OwnPtr<T>& src) {
    //     dst = src.raw;
    //     return dst;
    // }

    template <typename T>
	inline int findIdx(const std::vector<OwnPtr<T>>& v, const T* ptr) {
        int idx = 0;
		for( auto& op : v ) {
            if( op.raw == ptr ) {
                return idx;
            }
            idx++;
        }
		return -1;
	}
}

#endif