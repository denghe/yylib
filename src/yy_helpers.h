#pragma once
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <chrono>
#include <optional>
#include <variant>
#include <array>
#include <tuple>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include <cstddef>
#include <cstring>              // memcmp
#include <ctime>                // std::tm std::strftime

#include <cstdint>
#include <cassert>

#ifdef _WIN32
#include <intrin.h>             // _BitScanReverseXXXX _byteswap_XXXX
#endif
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#	define NOMINMAX
#	define NODRAWTEXT
//#	define NOGDI                // d3d9 maybe need it
#	define NOBITMAP
#	define NOMCX
#	define NOSERVICE
#	define NOHELP
#	define WIN32_LEAN_AND_MEAN
#   include <WinSock2.h>
#	include <Windows.h>
#else
#	include <unistd.h>          // for usleep
#endif

#ifndef _WIN32
#include <arpa/inet.h>          // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN
#endif


using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

#ifndef YY_NOINLINE
#   ifndef NDEBUG
#       define YY_NOINLINE
#       define YY_FORCE_INLINE
#       define YY_INLINE
#   else
#       ifdef _MSC_VER
#           define YY_NOINLINE __declspec(noinline)
#           define YY_FORCE_INLINE __forceinline
#           define YY_INLINE __forceinline
#       else
#           define YY_NOINLINE __attribute__((noinline))
#           define YY_FORCE_INLINE __attribute__((always_inline))
#           define YY_INLINE __attribute__((always_inline))
#       endif
#   endif
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define __LITTLE_ENDIAN__
#   elif __BYTE_ORDER == __BIG_ENDIAN
#       define __BIG_ENDIAN__
#   elif _WIN32
#       define __LITTLE_ENDIAN__
#   endif
#endif

#ifndef YY_STRINGIFY
#	define YY_STRINGIFY(x)  YY_STRINGIFY_(x)
#	define YY_STRINGIFY_(x)  #x
#endif

#if defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL) || defined(__x86_64__) || defined(_M_X64)
#    define YY_ARCH_IA
#endif

#if defined(__LP64__) || __WORDSIZE == 64 || defined(_WIN64) || defined(_M_X64)
#    define YY_ARCH_64
#endif

#ifdef _MSC_VER
#    define YY_ALIGN2( x )		    __declspec(align(2)) x
#    define YY_ALIGN4( x )		    __declspec(align(4)) x
#    define YY_ALIGN8( x )		    __declspec(align(8)) x
#    define YY_ALIGN16( x )		    __declspec(align(16)) x
#    define YY_ALIGN32( x )		    __declspec(align(32)) x
#    define YY_ALIGN64( x )		    __declspec(align(64)) x
#else
#    define YY_ALIGN2( x )          x __attribute__ ((aligned (2)))
#    define YY_ALIGN4( x )          x __attribute__ ((aligned (4)))
#    define YY_ALIGN8( x )          x __attribute__ ((aligned (8)))
#    define YY_ALIGN16( x )         x __attribute__ ((aligned (16)))
#    define YY_ALIGN32( x )         x __attribute__ ((aligned (32)))
#    define YY_ALIGN64( x )         x __attribute__ ((aligned (64)))
#endif

#ifdef _MSC_VER
#    define YY_LIKELY(x)                 (x)
#    define YY_UNLIKELY(x)               (x)
#else
#    define YY_UNLIKELY(x)               __builtin_expect((x), 0)
#    define YY_LIKELY(x)                 __builtin_expect((x), 1)
#endif

#ifndef _countof
template<typename T, size_t N>
size_t _countof(T const (&arr)[N]) {
    return N;
}
#endif

#ifndef _offsetof
#define _offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - _offsetof(type, member)))
#endif

#ifndef _WIN32
inline void Sleep(int const& ms) {
    usleep(ms * 1000);
}
#endif

#define YY_SIMPLE_STRUCT_DEFAULT_CODES(T)\
T() = default;\
T(T const&) = default;\
T(T &&) = default;\
T& operator=(T const&) = default;\
T& operator=(T &&) = default;

// stackless 协程相关
// 当前主要用到这些宏。只有 lineNumber 一个特殊变量名要求
#define COR_BEGIN	switch (lineNumber) { case 0:
#define COR_YIELD	return __LINE__; case __LINE__:;
#define COR_EXIT	return 0;
#define COR_END		} return 0;
/*
    int lineNumber = 0;
    int Update() {
        COR_BEGIN
            // COR_YIELD
        COR_END
    }
    ... lineNumber = Update();
*/

#define YY_ENUM_OPERATOR_EXT( EnumTypeName )                                                                    \
inline EnumTypeName operator+(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)	{			\
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + b);											\
}                                                                                                               \
inline EnumTypeName operator+(EnumTypeName const &a, EnumTypeName const &b) {                                   \
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + (std::underlying_type_t<EnumTypeName>)(b));	\
}                                                                                                               \
inline EnumTypeName operator-(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)	{			\
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) - b);											\
}                                                                                                               \
inline std::underlying_type_t<EnumTypeName> operator-(EnumTypeName const &a, EnumTypeName const &b)	{			\
    return (std::underlying_type_t<EnumTypeName>)(a) - (std::underlying_type_t<EnumTypeName>)(b);				\
}                                                                                                               \
inline EnumTypeName operator++(EnumTypeName &a) {                                                               \
    a = EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + 1);											\
    return a;                                                                                                   \
}

/*
SFINAE check menber function exists
sample：

YY_HAS_FUNC( xxxxxxxxfunc_checker, FuncName, RT ( T::* )( params... ) const );
*/
#define YY_HAS_FUNC( CN, FN, FT )   \
template<typename CT>                                                               \
class CN {                                                                          \
    template<typename T, FT> struct FuncMatcher;                                    \
    template<typename T> static char HasFunc( FuncMatcher<T, &T::FN>* );            \
    template<typename T> static int HasFunc( ... );                                 \
public:                                                                             \
    static const bool value = sizeof( HasFunc<CT>( nullptr ) ) == sizeof(char);     \
}

#define YY_HAS_TYPEDEF( TN ) \
template<typename, typename = void> struct HasTypedef_##TN : std::false_type {}; \
template<typename T> struct HasTypedef_##TN<T, std::void_t<typename T::TN>> : std::true_type {}; \
template<typename T> constexpr bool TN = HasTypedef_##TN<T>::value;


namespace yy {

    /************************************************************************************/
    // std::is_pod 的自定义扩展, 用于标识一个类可以在容器中被 memcpy | memmove

    template<typename T, typename ENABLED = void>
    struct IsPod : std::false_type {
    };
    template<typename T>
    struct IsPod<T, std::enable_if_t<std::is_standard_layout_v<T> && std::is_trivial_v<T>>> : std::true_type {
    };
    template<typename T> constexpr bool IsPod_v = IsPod<T>::value;


    /************************************************************************************/
    // Is 系列


    template<typename T, typename = void>
    struct IsLiteral : std::false_type {
    };
    template<size_t L>
    struct IsLiteral<char[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L>
    struct IsLiteral<char const[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L>
    struct IsLiteral<char const (&)[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<typename T>
    constexpr bool IsLiteral_v = IsLiteral<T>::value;
    template<typename T>
    constexpr size_t LiteralLen = IsLiteral<T>::len;


    template<typename T>
    struct IsOptional : std::false_type {
    };
    template<typename T>
    struct IsOptional<std::optional<T>> : std::true_type {
    };
    template<typename T>
    struct IsOptional<std::optional<T> &> : std::true_type {
    };
    template<typename T>
    struct IsOptional<std::optional<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsOptional_v = IsOptional<T>::value;


    template<typename T>
    struct IsVector : std::false_type {
    };
    template<typename T>
    struct IsVector<std::vector<T>> : std::true_type {
    };
    template<typename T>
    struct IsVector<std::vector<T> &> : std::true_type {
    };
    template<typename T>
    struct IsVector<std::vector<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsVector_v = IsVector<T>::value;


    template<typename>
    struct IsTuple : std::false_type {
    };
    template<typename ...T>
    struct IsTuple<std::tuple<T...>> : std::true_type {
    };
    template<typename ...T>
    struct IsTuple<std::tuple<T...> &> : std::true_type {
    };
    template<typename ...T>
    struct IsTuple<std::tuple<T...> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsTuple_v = IsTuple<T>::value;


    template<typename>
    struct IsVariant : std::false_type {
    };
    template<typename ...T>
    struct IsVariant<std::variant<T...>> : std::true_type {
    };
    template<typename ...T>
    struct IsVariant<std::variant<T...>&> : std::true_type {
    };
    template<typename ...T>
    struct IsVariant<std::variant<T...> const&> : std::true_type {
    };
    template<typename T>
    constexpr bool IsVariant_v = IsVariant<T>::value;


    template<typename T>
    struct IsUnique : std::false_type {
    };
    template<typename T>
    struct IsUnique<std::unique_ptr<T>> : std::true_type {
    };
    template<typename T>
    struct IsUnique<std::unique_ptr<T> &> : std::true_type {
    };
    template<typename T>
    struct IsUnique<std::unique_ptr<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsUnique_v = IsUnique<T>::value;


    template<typename T>
    struct IsUnorderedSet : std::false_type {
    };
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T>> : std::true_type {
    };
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T> &> : std::true_type {
    };
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsUnorderedSet_v = IsUnorderedSet<T>::value;


    template<typename T>
    struct IsSet : std::false_type {
    };
    template<typename T>
    struct IsSet<std::set<T>> : std::true_type {
    };
    template<typename T>
    struct IsSet<std::set<T> &> : std::true_type {
    };
    template<typename T>
    struct IsSet<std::set<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsSet_v = IsSet<T>::value;


    template<typename T>
    constexpr bool IsSetSeries_v = IsSet_v<T> || IsUnorderedSet_v<T>;



    template<typename T>
    struct IsQueue : std::false_type {
    };
    template<typename T>
    struct IsQueue<std::queue<T>> : std::true_type {
    };
    template<typename T>
    struct IsQueue<std::queue<T> &> : std::true_type {
    };
    template<typename T>
    struct IsQueue<std::queue<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsQueue_v = IsQueue<T>::value;

    template<typename T>
    struct IsDeque : std::false_type {
    };
    template<typename T>
    struct IsDeque<std::deque<T>> : std::true_type {
    };
    template<typename T>
    struct IsDeque<std::deque<T> &> : std::true_type {
    };
    template<typename T>
    struct IsDeque<std::deque<T> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsDeque_v = IsDeque<T>::value;

    template<typename T>
    constexpr bool IsQueueSeries_v = IsQueue_v<T> || IsDeque_v<T>;



    template<typename T>
    struct IsUnorderedMap : std::false_type {
        typedef void PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V>> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V> &> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V> const &> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename T>
    constexpr bool IsUnorderedMap_v = IsUnorderedMap<T>::value;
    template<typename T>
    using UnorderedMap_Pair_t = typename IsUnorderedMap<T>::PT;


    template<typename T>
    struct IsMap : std::false_type {
        typedef void PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V>> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V> &> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V> const &> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename T>
    constexpr bool IsMap_v = IsMap<T>::value;
    template<typename T>
    using Map_Pair_t = typename IsMap<T>::PT;


    template<typename T>
    constexpr bool IsMapSeries_v = IsUnorderedMap_v<T> || IsMap_v<T>;
    template<typename T>
    using MapSeries_Pair_t = std::conditional_t<IsUnorderedMap_v<T>, UnorderedMap_Pair_t<T>, Map_Pair_t<T>>;


    template<typename T>
    struct IsPair : std::false_type {
    };
    template<typename F, typename S>
    struct IsPair<std::pair<F, S>> : std::true_type {
    };
    template<typename F, typename S>
    struct IsPair<std::pair<F, S> &> : std::true_type {
    };
    template<typename F, typename S>
    struct IsPair<std::pair<F, S> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsPair_v = IsPair<T>::value;


    template<typename T>
    struct IsArray : std::false_type {
    };
    template<typename T, size_t S>
    struct IsArray<std::array<T, S>> : std::true_type {
    };
    template<typename T, size_t S>
    struct IsArray<std::array<T, S> &> : std::true_type {
    };
    template<typename T, size_t S>
    struct IsArray<std::array<T, S> const &> : std::true_type {
    };
    template<typename T>
    constexpr bool IsArray_v = IsArray<T>::value;


    template<typename, typename = void>
    struct IsContainer : std::false_type {
    };
    template<typename T>
    struct IsContainer<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>>
            : std::true_type {
    };
    template<typename T>
    constexpr bool IsIsContainer_v = IsContainer<T>::value;




    /************************************************************************************/
    // 基础类型 检查( 数值, enum, string, Data, 以及包裹容器的这些类型 )

    template<typename T, typename ENABLED = void>
    struct IsBaseDataType : std::false_type {
    };
    template<typename T> constexpr bool IsBaseDataType_v = IsBaseDataType<T>::value;

    struct Span;
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<std::is_arithmetic_v<T>
                                              || std::is_enum_v<T>
                                              || IsLiteral_v<T>
                                              || std::is_same_v<std::string, std::decay_t<T>>
                                              || std::is_same_v<std::string_view, std::decay_t<T>>
                                              || std::is_base_of_v<Span, T>
    >> : std::true_type {
    };

    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsOptional_v<T> && IsBaseDataType_v<typename T::value_type> >> : std::true_type {
    };
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsVector_v<T> && IsBaseDataType_v<typename T::value_type> >> : std::true_type {
    };
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsSetSeries_v<T> && IsBaseDataType_v<typename T::value_type> >> : std::true_type {
    };
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsPair_v<T> && IsBaseDataType_v<typename T::first_type> && IsBaseDataType_v<typename T::second_type> >> : std::true_type {
    };
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsMapSeries_v<T> && IsBaseDataType_v<typename T::key_type> && IsBaseDataType_v<typename T::value_type> >> : std::true_type {
    };

    // tuple 得遍历每个类型来判断
    template<typename Tuple, typename = std::enable_if_t<IsTuple_v<Tuple>>>
    struct TupleIsBaseDataType {
        template<std::size_t index>
        static constexpr bool Check__() {
            if constexpr (index == std::tuple_size_v<Tuple>) {
                return true;
            } else {
                return IsBaseDataType_v<std::tuple_element_t<index, Tuple>> ? Check__<index + 1>() : false;
            }
        }

        static constexpr bool Check() {
            return Check__<0>();
        }
    };

    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsTuple_v<T> && TupleIsBaseDataType<T>::Check() >> : std::true_type {
    };

    /************************************************************************************/
    // 判断 tuple 里是否存在某种数据类型

    template<typename T, typename Tuple>
    struct HasType;

    template<typename T>
    struct HasType<T, std::tuple<>> : std::false_type {
    };

    template<typename T, typename U, typename... Ts>
    struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {
    };

    template<typename T, typename... Ts>
    struct HasType<T, std::tuple<T, Ts...>> : std::true_type {
    };

    template<typename T, typename Tuple>
    using TupleContainsType = typename HasType<T, Tuple>::type;

    template<typename T, typename Tuple>
    constexpr bool TupleContainsType_v = TupleContainsType<T, Tuple>::value;


    /************************************************************************************/
    // 计算某类型在 tuple 里是第几个

    template<class T, class Tuple>
    struct TupleTypeIndex;

    template<class T, class...TS>
    struct TupleTypeIndex<T, std::tuple<T, TS...>> {
        static const size_t value = 0;
    };

    template<class T, class U, class... TS>
    struct TupleTypeIndex<T, std::tuple<U, TS...>> {
        static const size_t value = 1 + TupleTypeIndex<T, std::tuple<TS...>>::value;
    };

    template<typename T, typename Tuple>
    constexpr size_t TupleTypeIndex_v = TupleTypeIndex<T, Tuple>::value;
    
    
    /************************************************************************************/
    // 获取指定下标参数
    
    template <int I, typename...Args>
    decltype(auto) GetAt(Args&&...args) {
        return std::get<I>(std::forward_as_tuple(args...));
    }


    /************************************************************************************/
    // FuncR_t   FuncA_t  FuncC_t  lambda / function 类型拆解

    template<typename T, class = void>
    struct FuncTraits;

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        //using A3 = std::tuple<RefC_t<Args>...>;
        using C = void;
    };

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        //using A3 = std::tuple<RefC_t<Args>...>;
        using C = void;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        //using A3 = std::tuple<RefC_t<Args>...>;
        using C = CT;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...) const> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        //using A3 = std::tuple<RefC_t<Args>...>;
        using C = CT;
    };

    template<typename T>
    struct FuncTraits<T, std::void_t<decltype(&T::operator())> >
            : public FuncTraits<decltype(&T::operator())> {
    };

    template<typename T>
    using FuncR_t = typename FuncTraits<T>::R;
    template<typename T>
    using FuncA_t = typename FuncTraits<T>::A;
    template<typename T>
    using FuncA2_t = typename FuncTraits<T>::A2;
    //template<typename T>
    //using FuncA3_t = typename FuncTraits<T>::A3;
    template<typename T>
    using FuncC_t = typename FuncTraits<T>::C;


    /************************************************************************************/
    // 针对模板基类，检查某类型是否其派生类. 用法: XX_IsTemplateOf(BT, T)::value

    struct IsTemplateOf {
        template <template <class> class TM, class T> static std::true_type  check(TM<T>);
        template <template <class> class TM>          static std::false_type check(...);
        template <template <int>   class TM, int N>   static std::true_type  check(TM<N>);
        template <template <int>   class TM>          static std::false_type check(...);
    };
    #define XX_IsTemplateOf(TM, ...) decltype(::xx::IsTemplateOf::check<TM>(std::declval<__VA_ARGS__>()))


	/************************************************************************************/
	// weak_ptr 系列

	template<typename T, typename U>
	std::weak_ptr<T> AsWeak(std::shared_ptr<U> const& v) noexcept {
		return std::weak_ptr<T>(std::shared_ptr<T>(v));
	}

	template<typename T>
	std::weak_ptr<T> ToWeak(std::shared_ptr<T> const& v) noexcept {
		return std::weak_ptr<T>(v);
	}


    /************************************************************************************/
    // unique_ptr 系列

    template<typename T, typename ...Args>
    std::unique_ptr<T> MakeU(Args &&...args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename ...Args>
    std::unique_ptr<T> TryMakeU(Args &&...args) noexcept {
        try {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        catch (...) {
            return std::unique_ptr<T>();
        }
    }


    /************************************************************************************/
    // malloc 系列

    template<typename T>
    T *Malloc() {
        return (T *) malloc(sizeof(T));
    }

    template<typename T>
    T *&MallocTo(T *&v) {
        v = (T *) malloc(sizeof(T));
        return v;
    }



    /***********************************************************************************/
    // MaxSizeof
    /***********************************************************************************/

    template<typename T, typename... Args>
    struct MaxSizeof {
        static const size_t value = sizeof(T) > MaxSizeof<Args...>::value
                                    ? sizeof(T)
                                    : MaxSizeof<Args...>::value;
    };
    template<typename T>
    struct MaxSizeof<T> {
        static const size_t value = sizeof(T);
    };

    template<typename T, typename... Args>
    constexpr size_t MaxSizeof_v = MaxSizeof<T, Args...>::value;




    /***********************************************************************************/
    // Hash for Dict, tsl::xxxxxxmap. 顺序数字 key 简单绕开 std::hash
    /***********************************************************************************/

    template<typename T, typename ENABLED = void>
    struct Hash {
        inline size_t operator()(T const& k) const {
            return std::hash<T>{}(k);
        }
    };

    // 适配所有数字
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        inline size_t operator()(T const& k) const {
            return (size_t)k;
        }
    };

    // 适配 enum
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_enum_v<T>>> {
        inline size_t operator()(T const& k) const {
            return (std::underlying_type_t<T>)k;
        }
    };

    // 适配 指针( 除以 4/8 )
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_pointer_v<T>>> {
        inline size_t operator()(T const& k) const {
            if constexpr (sizeof(size_t) == 8) return (size_t)k / 8;
            else return (size_t)k / 4;
        }
    };




    /************************************************************************************/
    // TypeName_v

    // 注意：这部分内容需要跟随各种编译器及时更新调整

    namespace Detail {
        template<typename... T>
        constexpr auto NameOf() noexcept {
#if defined(__clang__)
            /* 输出前后缀：
auto xx::Detail::NameOf() [T = <
>]
*/
            return std::string_view{ __PRETTY_FUNCTION__ + 32, sizeof(__PRETTY_FUNCTION__) - 35 };
#elif defined(__GNUC__)
            /* 输出前后缀：
constexpr auto xx::Detail::NameOf() [with T = {
}]
*/
            return std::string_view{__PRETTY_FUNCTION__ + 47, sizeof(__PRETTY_FUNCTION__) - 50};
#elif defined(_MSC_VER) && _MSC_VER >= 1920
            /* 输出前后缀：
auto __cdecl xx::Detail::NameOf<
>(void) noexcept
*/
            return std::string_view{ __FUNCSIG__ + 32, sizeof(__FUNCSIG__) - 49 };
#else
            static_assert(false, "unsupported compiler");
#endif
        }
    }
    template<typename... T>
    inline constexpr auto TypeName_v = Detail::NameOf<T...>();

    /************************************************************************************/
    // IsLambda_v

    // 注意：这部分内容需要跟随各种编译器及时更新调整

    namespace Detail {
        template<typename T>
        constexpr bool IsLambda() {
            constexpr std::string_view tn = TypeName_v<T>;
#if defined(__clang__)
            // lambda 输出长相: (lambda at /full_path/fileName.ext:line:colum)
            return tn.rfind("(lambda at ", 0) == 0 && *tn.rbegin() == ')' && tn.find(':') > tn.rfind('(');
#elif defined(__GNUC__)
            // lambda 输出长相: ... <lambda( ... )>
            if constexpr (tn.size() < 10 || tn[tn.size() - 1] != '>' || tn[tn.size() - 2] != ')') return false;
            auto at = tn.size() - 3;
            for (std::size_t i = 1; i; --at) {
                i += tn[at = tn.find_last_of("()", at)] == '(' ? -1 : 1;
            }
            return at >= 6 && tn.substr(at - 6, 7) == "<lambda";
#elif defined(_MSC_VER) && _MSC_VER >= 1920
            // lambda 输出长相: class ????::<lambda_????>
            return tn.rfind("class ", 0) == 0 && tn.find("::<lambda_", 0) != tn.npos && *tn.rbegin() == '>';
#else
#error unsupported compiler!
#endif
        }
    }

    template<typename T>
    struct IsLambda : std::integral_constant<bool, Detail::IsLambda<T>()> {
    };

    template<typename T>
    inline constexpr bool IsLambda_v = IsLambda<T>::value;





    /************************************************************************************/
    // Scope Guard( F == lambda )

    template<class F>
    auto MakeScopeGuard(F &&f) noexcept {
        struct ScopeGuard {
            F f;
            bool cancel;

            explicit ScopeGuard(F &&f) noexcept: f(std::move(f)), cancel(false) {}

            ~ScopeGuard() noexcept { if (!cancel) { f(); }}

            inline void Cancel() noexcept { cancel = true; }

            inline void operator()(bool cancel = false) {
                f();
                this->cancel = cancel;
            }
        };
        return ScopeGuard(std::forward<F>(f));
    }


    template<class F>
    auto MakeSimpleScopeGuard(F&& f) noexcept {
        struct SG { F f; SG(F&& f) noexcept : f(std::move(f)) {} ~SG() { f(); } };
        return SG(std::forward<F>(f));
    }


    /************************************************************************************/
    // time_point <--> .net DateTime.Now.ToUniversalTime().Ticks converts

    // 经历时间精度: 秒后 7 个 0( 这是 windows 下最高精度. android/ios 会低1个0的精度 )
    typedef std::chrono::duration<long long, std::ratio<1LL, 10000000LL>> duration_10m;

    // 时间点 转 epoch (精度为秒后 7 个 0)
    inline int64_t TimePointToEpoch10m(std::chrono::system_clock::time_point const &val) noexcept {
        return std::chrono::duration_cast<duration_10m>(val.time_since_epoch()).count();
    }

    inline int64_t TimePointToEpoch10m(std::chrono::steady_clock::time_point const &val) noexcept {
        return std::chrono::duration_cast<duration_10m>(val.time_since_epoch()).count();
    }

    //  epoch (精度为秒后 7 个 0) 转 时间点
    inline std::chrono::system_clock::time_point Epoch10mToTimePoint(int64_t const &val) noexcept {
        return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration_10m(val)));
    }


    // 得到当前时间点
    inline std::chrono::system_clock::time_point Now() noexcept {
        return std::chrono::system_clock::now();
    }

    inline std::chrono::system_clock::time_point NowTimePoint() noexcept {
        return std::chrono::system_clock::now();
    }

    inline std::chrono::steady_clock::time_point NowSteadyTimePoint() noexcept {
        return std::chrono::steady_clock::now();
    }

    // 得到当前时间点的 epoch ticks(精度为秒后 7 个 0)
    inline int64_t NowEpoch10m() noexcept {
        return TimePointToEpoch10m(NowTimePoint());
    }

    // 得到当前时间点的 epoch 微妙
    inline int64_t NowEpochMicroseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 毫秒
    inline int64_t NowEpochMilliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 秒
    inline double NowEpochSeconds() noexcept {
        return (double) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000.0;
    }

    // 得到当前时间点的 epoch 秒 更新并返回和 last 的时间差. last 可用 NowEpochSeconds 初始化
    inline double NowEpochSeconds(double &last) noexcept {
        auto now = NowEpochSeconds();
        auto rtv = now - last;
        last = now;
        return rtv;
    }

    // epoch (精度为秒后 7 个 0) 转为 .Net DateTime Utc Ticks
    inline int64_t Epoch10mToUtcDateTimeTicks(int64_t const &val) noexcept {
        return val + 621355968000000000LL;
    }

    // .Net DateTime Utc Ticks 转为 epoch (精度为秒后 7 个 0)
    inline int64_t UtcDateTimeTicksToEpoch10m(int64_t const &val) noexcept {
        return val - 621355968000000000LL;
    }

    // 时间点 转 epoch (精度为秒)
    inline int32_t TimePointToEpoch(std::chrono::system_clock::time_point const &val) noexcept {
        return (int32_t) (val.time_since_epoch().count() / 10000000);
    }

    //  epoch (精度为秒) 转 时间点
    inline std::chrono::system_clock::time_point EpochToTimePoint(int32_t const &val) noexcept {
        return std::chrono::system_clock::time_point(std::chrono::system_clock::time_point::duration((int64_t) val * 10000000));
    }

    // 得到当前时间点的 epoch ticks(精度为秒后 7 个 0)
    inline int64_t NowSteadyEpoch10m() noexcept {
        return TimePointToEpoch10m(NowSteadyTimePoint());
    }

    // 得到当前时间点的 epoch 微妙
    inline int64_t NowSteadyEpochMicroseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 毫秒
    inline int64_t NowSteadyEpochMilliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 秒
    inline double NowSteadyEpochSeconds() noexcept {
        return (double) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1000000.0;
    }

    // 得到当前时间点的 epoch 秒 更新并返回和 last 的时间差. last 可用 NowEpochSeconds 初始化
    inline double NowSteadyEpochSeconds(double &last) noexcept {
        auto now = NowSteadyEpochSeconds();
        auto rtv = now - last;
        last = now;
        return rtv;
    }

    // 时间点转为本地时间 string ( 填充 )
    template<typename T>
    inline void AppendTimePoint_Local(std::string &s, T const &tp) {
        auto &&t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        auto bak = s.size();
        s.resize(s.size() + 30);
        auto len = std::strftime(&s[bak], 30, "%Y-%m-%d %H:%M:%S", &tm);
        s.resize(bak + len);
    }

    // 时间点转为本地时间 string 返回
    template<typename T>
    inline std::string TimePointToString_Local(T const &tp) {
        std::string s;
        AppendTimePoint_Local(s, tp);
        return s;
    }

}
