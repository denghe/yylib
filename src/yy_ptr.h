#pragma once
#include "yy_helpers.h"

// 类似 std::shared_ptr / weak_ptr，非线程安全，weak_ptr 提供了无损 shared_count 检测功能以方便直接搞事情
// 如果需要跨线程访问, 确保没有别的地方胡乱引用, move 到目标容器. 或者干脆 clone 一份独立的传递

namespace yy {

    /************************************************************************************/
    // Make 时会在内存块头部附加

    struct shared_ptr_header {
        uint32_t shared_count;
        uint32_t weak_count;

        template<typename T>
        void init() {
            shared_count = 1;
            weak_count = 0;
        }
    };


    // 适配路由
    template<typename T, typename ENABLED = void>
    struct shared_ptr_header_switcher {
        using type = shared_ptr_header;
    };

    template<typename T, typename ENABLED = void>
    using shared_ptr_header_t = typename shared_ptr_header_switcher<T>::type;


    /************************************************************************************/
    // std::shared_ptr like

    template<typename T>
    struct weak_ptr;

    template<typename T>
    struct shared_ptr {
        using HeaderType = shared_ptr_header_t<T>;
        using ElementType = T;
        T *pointer = nullptr;

        YY_INLINE operator T *const &() const noexcept {
            return pointer;
        }

        YY_INLINE operator T *&() noexcept {
            return pointer;
        }

        YY_INLINE T *const &operator->() const noexcept {
            return pointer;
        }

        YY_INLINE T const &Value() const noexcept {
            return *pointer;
        }

        YY_INLINE T &Value() noexcept {
            return *pointer;
        }

        [[nodiscard]] YY_INLINE explicit operator bool() const noexcept {
            return pointer != nullptr;
        }

        [[nodiscard]] YY_INLINE bool Empty() const noexcept {
            return pointer == nullptr;
        }

        [[nodiscard]] YY_INLINE bool HasValue() const noexcept {
            return pointer != nullptr;
        }

        [[nodiscard]] YY_INLINE uint32_t GetSharedCount() const noexcept {
            if (!pointer) return 0;
            return GetHeader()->shared_count;
        }

        [[nodiscard]] YY_INLINE uint32_t GetWeakCount() const noexcept {
            if (!pointer) return 0;
            return GetHeader()->weak_count;
        }

        // unsafe
        [[nodiscard]] YY_INLINE HeaderType *GetHeader() const noexcept {
            return ((HeaderType *) pointer - 1);
        }

        void Reset() {
            if (pointer) {
                auto h = GetHeader();
                assert(h->shared_count);
                // 不能在这里 -1, 这将导致成员 weak 指向自己时触发 free
                if (h->shared_count == 1) {
                    pointer->~T();
                    pointer = nullptr;
                    if (h->weak_count == 0) {
                        free(h);
                    } else {
                        h->shared_count = 0;
                    }
                } else {
                    --h->shared_count;
                    pointer = nullptr;
                }
            }
        }

        template<typename U>
        void Reset(U *const &ptr) {
            static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
            if (pointer == ptr) return;
            Reset();
            if (ptr) {
                pointer = ptr;
                ++((HeaderType *) ptr - 1)->shared_count;
            }
        }

        YY_INLINE ~shared_ptr() {
            Reset();
        }

        shared_ptr() = default;

        template<typename U>
        YY_INLINE shared_ptr(U *const &ptr) {
            static_assert(std::is_base_of_v<T, U>);
            pointer = ptr;
            if (ptr) {
                ++((HeaderType *) ptr - 1)->shared_count;
            }
        }

        YY_INLINE shared_ptr(T *const &ptr) {
            pointer = ptr;
            if (ptr) {
                ++((HeaderType *) ptr - 1)->shared_count;
            }
        }

        template<typename U>
        YY_INLINE shared_ptr(shared_ptr<U> const &o) : shared_ptr(o.pointer) {}

        YY_INLINE shared_ptr(shared_ptr const &o) : shared_ptr(o.pointer) {}

        template<typename U>
        YY_INLINE shared_ptr(shared_ptr<U> &&o) noexcept {
            static_assert(std::is_base_of_v<T, U>);
            pointer = o.pointer;
            o.pointer = nullptr;
        }

        YY_INLINE shared_ptr(shared_ptr &&o) noexcept {
            pointer = o.pointer;
            o.pointer = nullptr;
        }

        template<typename U>
        YY_INLINE shared_ptr &operator=(U *const &ptr) {
            static_assert(std::is_base_of_v<T, U>);
            Reset(ptr);
            return *this;
        }

        YY_INLINE shared_ptr &operator=(T *const &ptr) {
            Reset(ptr);
            return *this;
        }

        template<typename U>
        YY_INLINE shared_ptr &operator=(shared_ptr<U> const &o) {
            static_assert(std::is_base_of_v<T, U>);
            Reset(o.pointer);
            return *this;
        }

        YY_INLINE shared_ptr &operator=(shared_ptr const &o) {
            Reset(o.pointer);
            return *this;
        }

        template<typename U>
        YY_INLINE shared_ptr &operator=(shared_ptr<U> &&o) {
            static_assert(std::is_base_of_v<T, U>);
            Reset();
            std::swap(pointer, (*(shared_ptr *) &o).pointer);
            return *this;
        }

        YY_INLINE shared_ptr &operator=(shared_ptr &&o) {
            std::swap(pointer, o.pointer);
            return *this;
        }

        template<typename U>
        YY_INLINE bool operator==(shared_ptr<U> const &o) const noexcept {
            return pointer == o.pointer;
        }

        template<typename U>
        YY_INLINE bool operator!=(shared_ptr<U> const &o) const noexcept {
            return pointer != o.pointer;
        }

        // 有条件的话尽量使用 ObjManager 的 As, 避免发生 dynamic_cast
        template<typename U>
        YY_INLINE shared_ptr<U> As() const noexcept {
            if constexpr (std::is_same_v<U, T>) {
                return *this;
            } else if constexpr (std::is_base_of_v<U, T>) {
                return pointer;
            } else {
                return dynamic_cast<U *>(pointer);
            }
        }

        // unsafe: 直接硬转返回. 使用前通常会根据 typeId 进行合法性检测
        template<typename U>
        YY_INLINE shared_ptr<U> &ReinterpretCast() const noexcept {
            return *(shared_ptr<U> *) this;
        }

        struct weak_ptr<T> ToWeak() const noexcept;

        // 填充式 make
        template<typename...Args>
        shared_ptr &Emplace(Args &&...args);

        // singleton convert to std::shared_ptr ( usually for thread safe )
        std::shared_ptr<T> ToSharedPtr() noexcept {
            assert(GetSharedCount() == 1 && GetWeakCount() == 0);
            auto bak = pointer;
            pointer = nullptr;
            return std::shared_ptr<T>(bak, [](T *p) {
                p->~T();
                free((HeaderType *) p - 1);
            });
        }
    };

    /************************************************************************************/
    // std::weak_ptr like

    template<typename T>
    struct weak_ptr {
        using HeaderType = shared_ptr_header_t<T>;
        using ElementType = T;
        HeaderType *h = nullptr;

        [[nodiscard]] YY_INLINE uint32_t GetSharedCount() const noexcept {
            if (!h) return 0;
            return h->shared_count;
        }

        [[nodiscard]] YY_INLINE uint32_t GetWeakCount() const noexcept {
            if (!h) return 0;
            return h->weak_count;
        }
 
        [[nodiscard]] YY_INLINE explicit operator bool() const noexcept {
            return h && h->shared_count;
        }

        // unsafe: 直接计算出指针
        [[nodiscard]] YY_INLINE T *GetPointer() const {
            return (T *) (h + 1);
        }

        YY_INLINE void Reset() {
            if (h) {
                if (h->weak_count == 1 && h->shared_count == 0) {
                    free(h);
                } else {
                    --h->weak_count;
                }
                h = nullptr;
            }
        }

        template<typename U>
        YY_INLINE void Reset(shared_ptr<U> const &s) {
            static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
            Reset();
            if (s.pointer) {
                h = ((HeaderType *) s.pointer - 1);
                ++h->weak_count;
            }
        }

        [[nodiscard]] YY_INLINE shared_ptr<T> Lock() const {
            if (h && h->shared_count) {
                auto p = h + 1;
                return *(shared_ptr<T> *) &p;
            }
            return {};
        }

        // unsafe 系列: 要安全使用，每次都 if 真 再调用这些函数 1 次。一次 if 多次调用的情景除非很有把握在期间 shared_ptr 不会析构，否则还是 Lock()
        [[nodiscard]] YY_INLINE ElementType *operator->() const noexcept {
            return (ElementType *) (h + 1);
        }

        [[nodiscard]] YY_INLINE ElementType const &Value() const noexcept {
            return *(ElementType *) (h + 1);
        }

        [[nodiscard]] YY_INLINE ElementType &Value() noexcept {
            return *(ElementType *) (h + 1);
        }

        YY_INLINE operator ElementType *() const noexcept {
            return (ElementType *) (h + 1);
        }

        template<typename U>
        YY_INLINE weak_ptr &operator=(shared_ptr<U> const &o) {
            static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
            Reset(o);
            return *this;
        }

        template<typename U>
        YY_INLINE weak_ptr(shared_ptr<U> const &o) {
            static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
            Reset(o);
        }

        YY_INLINE ~weak_ptr() {
            Reset();
        }

        weak_ptr() = default;

        YY_INLINE weak_ptr(weak_ptr const &o) {
            if ((h = o.h)) {
                ++o.h->weak_count;
            }
        }

        template<typename U>
        YY_INLINE weak_ptr(weak_ptr<U> const &o) {
            static_assert(std::is_base_of_v<T, U>);
            if ((h = o.h)) {
                ++o.h->weak_count;
            }
        }

        YY_INLINE weak_ptr(weak_ptr &&o) noexcept {
            h = o.h;
            o.h = nullptr;
        }

        template<typename U>
        YY_INLINE weak_ptr(weak_ptr<U> &&o) noexcept {
            static_assert(std::is_base_of_v<T, U>);
            h = o.h;
            o.h = nullptr;
        }

        YY_INLINE weak_ptr &operator=(weak_ptr const &o) {
            if (&o != this) {
                Reset(o.Lock());
            }
            return *this;
        }

        template<typename U>
        YY_INLINE weak_ptr &operator=(weak_ptr<U> const &o) {
            static_assert(std::is_base_of_v<T, U>);
            if ((void *) &o != (void *) this) {
                Reset(((weak_ptr *) (&o))->Lock());
            }
            return *this;
        }

        YY_INLINE weak_ptr &operator=(weak_ptr &&o) noexcept {
            std::swap(h, o.h);
            return *this;
        }
        // operator=(weak_ptr&& o) 没有模板实现，因为不确定交换 h 之后的类型是否匹配

        template<typename U>
        YY_INLINE bool operator==(weak_ptr<U> const &o) const noexcept {
            return h == o.h;
        }

        template<typename U>
        YY_INLINE bool operator!=(weak_ptr<U> const &o) const noexcept {
            return h != o.h;
        }
    };

    template<typename T>
    weak_ptr<T> shared_ptr<T>::ToWeak() const noexcept {
        if (pointer) {
            auto h = (HeaderType *) pointer - 1;
            return *(weak_ptr<T> *) &h;
        }
        return {};
    }

    template<typename T>
    template<typename...Args>
    shared_ptr<T> &shared_ptr<T>::Emplace(Args &&...args) {
        Reset();
        auto h = (HeaderType *) malloc(sizeof(HeaderType) + sizeof(T));
        h->init<T>();
        pointer = new(h + 1) T(std::forward<Args>(args)...);
        return *this;
    }


    /************************************************************************************/
    // helpers

    template<typename T>
    struct IsShared : std::false_type {
    };
    template<typename T>
    struct IsShared<shared_ptr<T>> : std::true_type {
    };
    template<typename T>
    struct IsShared<shared_ptr<T> &> : std::true_type {
    };
    template<typename T>
    struct IsShared<shared_ptr<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsShared_v = IsShared<T>::value;


    template<typename T>
    struct IsWeak : std::false_type {
    };
    template<typename T>
    struct IsWeak<weak_ptr<T>> : std::true_type {
    };
    template<typename T>
    struct IsWeak<weak_ptr<T> &> : std::true_type {
    };
    template<typename T>
    struct IsWeak<weak_ptr<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsWeak_v = IsWeak<T>::value;


    template<typename T, typename...Args>
    [[nodiscard]] shared_ptr<T> Make(Args &&...args) {
        shared_ptr<T> rtv;
        rtv.Emplace(std::forward<Args>(args)...);
        return rtv;
    }

    template<typename T, typename ...Args>
    shared_ptr<T> TryMake(Args &&...args) noexcept {
        try {
            return Make<T>(std::forward<Args>(args)...);
        }
        catch (...) {
            return shared_ptr<T>();
        }
    }

    template<typename T, typename ...Args>
    shared_ptr<T> &MakeTo(shared_ptr<T> &v, Args &&...args) {
        v = Make<T>(std::forward<Args>(args)...);
        return v;
    }

    template<typename T, typename ...Args>
    shared_ptr<T> &TryMakeTo(shared_ptr<T> &v, Args &&...args) noexcept {
        v = TryMake<T>(std::forward<Args>(args)...);
        return v;
    }

    template<typename T, typename U>
    shared_ptr<T> As(shared_ptr<U> const &v) noexcept {
        return v.template As<T>();
    }

    template<typename T, typename U>
    bool Is(shared_ptr<U> const &v) noexcept {
        return !v.template As<T>().Empty();
    }

    // unsafe
    template<typename T>
    shared_ptr<T> SharedFromThis(T *const &thiz) {
        return *(shared_ptr<T> *) &thiz;
    }

}

// 令 shared_ptr weak_ptr 支持放入 hash 容器
namespace std {
    template<typename T>
    struct hash<yy::shared_ptr<T>> {
        size_t operator()(yy::shared_ptr<T> const &v) const {
            return (size_t) v.pointer;
        }
    };

    template<typename T>
    struct hash<yy::weak_ptr<T>> {
        size_t operator()(yy::weak_ptr<T> const &v) const {
            return (size_t) v.h;
        }
    };
}
