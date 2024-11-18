/*
	imdongye@naver.com
	fst: 2024-08-27
	lst: 2024-08-27
*/
#ifndef __mecro_h_
#define __mecro_h_


namespace lim
{
    class NoCopy {
    private:
        NoCopy(const NoCopy&) = delete;
        NoCopy& operator=(const NoCopy&) = delete;
    public:
        NoCopy() = default;
        ~NoCopy() = default;
        NoCopy(NoCopy&&) noexcept  = default;
        NoCopy& operator=(NoCopy&&) noexcept = default;
    };

    class NoMove {
    private:
        NoMove(NoMove&&) noexcept  = delete;
        NoMove& operator=(NoMove&&) noexcept = delete;
    public:
        NoMove() = default;
        ~NoMove() = default;
        NoMove(const NoMove&) = default;
        NoMove& operator=(const NoMove&) = default;
    };

    class NoCopyAndMove {
    private:
        NoCopyAndMove(const NoCopyAndMove&) = delete;
        NoCopyAndMove& operator=(const NoCopyAndMove&) = delete;
        NoCopyAndMove(NoCopyAndMove&&) noexcept  = delete;
        NoCopyAndMove& operator=(NoCopyAndMove&&) noexcept = delete;
    public:
        NoCopyAndMove() = default;
        ~NoCopyAndMove() = default;
    };

    class OnlyStatic {
    private:
        OnlyStatic() = delete;
        ~OnlyStatic() = delete;
        OnlyStatic(const OnlyStatic&) = delete;
        OnlyStatic& operator=(const OnlyStatic&) = delete;
        OnlyStatic(OnlyStatic&&) noexcept  = delete;
        OnlyStatic& operator=(OnlyStatic&&) noexcept = delete;
    };
    

    template<typename T>
    class SingletonStatic : public NoCopyAndMove {
    private:
        inline static T instance;
    public:
        inline static T& get() {
            return instance;
        }
    };

    template<typename T>
    class SingletonDynamic : public NoCopyAndMove {
    private:
        inline static T* instance = nullptr;
    public:
        inline static void create() {
            assert( instance == nullptr );
            instance = new T();
        }
        inline static void destroy() {
            assert( instance );
            delete instance;
            instance = nullptr;
        }
        inline static T& get() {
            return *instance;
        }
    };
}


#endif