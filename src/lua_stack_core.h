#ifndef LUA_STACK_CORE_H
#define LUA_STACK_CORE_H

#include <cstddef>
#include <string>
#include <type_traits>
#include <luajit-2.0/lua.hpp>

namespace Ltl
{

namespace detail
{
// -----------------------------------------------------------------------------
// tags
// -----------------------------------------------------------------------------
struct numeric_tag {};
struct floating_point_tag {};
struct integral_tag {};
struct unsigned_tag {};
struct boolean_tag {};
struct string_tag {};
struct cstring_tag {};
struct pointer_tag {};

// this tag should not be specialized. it is similar in function to the
// fall-through 'default' switch case
struct default_tag {};

// -----------------------------------------------------------------------------
// trait definitions
// -----------------------------------------------------------------------------
template<typename T, typename Enable = void>
struct PushTrait
{ using tag = typename T::push_tag; };

template<typename T, typename Enable = void>
struct CastTrait
{ using tag = typename T::cast_tag; };

template<typename T, typename Enable = void>
struct TypeTrait
{ using tag = typename T::type_tag; };

template<typename T, typename Enable = void>
struct ZeroTrait
{ using tag = typename T::zero_tag; };

template<typename T, typename Enable = void>
struct NameTrait
{ using tag = typename T::name_tag; };

template<typename T, typename Enable = void>
struct LuaType
{ static constexpr int code = T::lua_type_code; };

// -----------------------------------------------------------------------------
// policy definitions
// -----------------------------------------------------------------------------
template<typename Tag>
struct PushPolicy {};

template<typename Tag>
struct CastPolicy {};

template<typename Tag>
struct TypePolicy
{
    template<typename T>
    static bool type(lua_State* L, int n)
    { return lua_type(L, n) == LuaType<T>::code; }
};

// default values
template<typename Tag>
struct ZeroPolicy
{
    template<typename T>
    static T zero(lua_State*, int)
    { return 0; }
};

// for generating informative error messages
template<typename Tag>
struct NamePolicy
{
    template<typename T>
    static std::string name(lua_State* L, int)
    { return lua_typename(L, LuaType<T>::code); }
};

// -----------------------------------------------------------------------------
// core policy specializations
// -----------------------------------------------------------------------------
// push
template<>
struct PushPolicy<floating_point_tag>
{
    template<typename T>
    static void push(lua_State* L, T v)
    { lua_pushnumber(L, v); }
};

template<>
struct PushPolicy<integral_tag>
{
    template<typename T>
    static void push(lua_State* L, T v)
    { lua_pushinteger(L, v); }
};

template<>
struct PushPolicy<pointer_tag>
{
    template<typename T>
    static void push(lua_State* L, T v)
    { lua_pushlightuserdata(L, v); }
};

template<>
struct PushPolicy<boolean_tag>
{
    template<typename T>
    static void push(lua_State* L, T v)
    { lua_pushboolean(L, v); }
};

template<>
struct PushPolicy<string_tag>
{
    template<typename T>
    static void push(lua_State* L, const T& v)
    { lua_pushlstring(L, v.c_str(), v.size()); }
};

template<>
struct PushPolicy<cstring_tag>
{
    template<typename T>
    static void push(lua_State* L, T v)
    { lua_pushstring(L, v); }
};

//cast
template<>
struct CastPolicy<floating_point_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    { return lua_tonumber(L, n); }
};

template<>
struct CastPolicy<integral_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    { return lua_tointeger(L, n); }
};

template<>
struct CastPolicy<pointer_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    {
        return static_cast<T>(
            const_cast<void*>(lua_topointer(L, n))
        );
    }
};

template<>
struct CastPolicy<boolean_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    { return lua_toboolean(L, n); }
};

template<>
struct CastPolicy<string_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    {
        size_t len = 0;
        return T(lua_tolstring(L, n, &len), len);
    }
};

template<>
struct CastPolicy<cstring_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    { return lua_tostring(L, n); }
};

//type
template<>
struct TypePolicy<unsigned_tag>
{
    template<typename T>
    static bool type(lua_State* L, int n)
    {
        if ( lua_type(L, n) != LuaType<T>::code )
            return false;

        auto v = lua_tointeger(L, n);
        return v >= 0;
    }
};

// zero
template<>
struct ZeroPolicy<string_tag>
{
    template<typename T>
    static T zero(lua_State*, int)
    { return ""; }
};

// name
template<>
struct NamePolicy<integral_tag>
{
    template<typename T>
    static std::string name(lua_State*, int)
    { return "integer"; }
};

template<>
struct NamePolicy<unsigned_tag>
{
    template<typename T>
    static std::string name(lua_State*, int)
    { return "unsigned"; }
};

template<>
struct NamePolicy<pointer_tag>
{
    template<typename T>
    static std::string name(lua_State*, int)
    { return "pointer"; }
};

// -----------------------------------------------------------------------------
// core trait specializations
// -----------------------------------------------------------------------------

// type traits helpers
template<typename T>
struct CTraits
{
    static constexpr size_t size = sizeof(T);
    static constexpr size_t integral_max_size = sizeof(lua_Integer);
    static constexpr size_t float_max_size = sizeof(lua_Number);

    static constexpr bool is_bool = std::is_same<T, bool>::value;

    static constexpr bool is_integral = std::is_integral<T>::value && !is_bool;

    static constexpr bool is_signed_int =
        is_integral && std::is_signed<T>::value && size <= integral_max_size;

    static constexpr bool is_unsigned_int =
        is_integral && std::is_unsigned<T>::value && size < integral_max_size;

    static constexpr bool is_int = is_signed_int || is_unsigned_int;

    static constexpr bool is_float =
        std::is_floating_point<T>::value && size <= float_max_size;

    static constexpr bool is_numeric = is_int || is_float;

    static constexpr bool is_string =
        std::is_same<T, const char*>::value ||
        std::is_same<T, std::string>::value;

    static constexpr bool is_pointer =
        std::is_pointer<T>::value &&
        !is_string; // handle const char* case

    static constexpr bool is_basic =
        is_numeric ||
        is_bool ||
        is_string ||
        is_pointer;
};

// push trait
template<typename T>
struct PushTrait<T, typename std::enable_if<CTraits<T>::is_float>::type>
{ using tag = floating_point_tag; };

template<typename T>
struct PushTrait<T, typename std::enable_if<CTraits<T>::is_int>::type>
{ using tag = integral_tag; };

template<typename T>
struct PushTrait<T, typename std::enable_if<CTraits<T>::is_pointer>::type>
{ using tag = pointer_tag; };

template<>
struct PushTrait<bool>
{ using tag = boolean_tag; };

template<>
struct PushTrait<std::string>
{ using tag = string_tag; };

template<>
struct PushTrait<const char*>
{ using tag = cstring_tag; };

// cast trait
template<typename T>
struct CastTrait<T, typename std::enable_if<CTraits<T>::is_float>::type>
{ using tag = floating_point_tag; };

template<typename T>
struct CastTrait<T, typename std::enable_if<CTraits<T>::is_int>::type>
{ using tag = integral_tag; };

template<typename T>
struct CastTrait<T, typename std::enable_if<CTraits<T>::is_pointer>::type>
{ using tag = pointer_tag; };

template<>
struct CastTrait<bool>
{ using tag = boolean_tag; };

template<>
struct CastTrait<std::string>
{ using tag = string_tag; };

template<>
struct CastTrait<const char*>
{ using tag = cstring_tag; };

// type trait
template<typename T>
struct TypeTrait<T, typename std::enable_if<
    CTraits<T>::is_basic &&
    !CTraits<T>::is_unsigned_int
    >::type>
{ using tag = default_tag; };

template<typename T>
struct TypeTrait<T, typename std::enable_if<CTraits<T>::is_unsigned_int>::type>
{ using tag = unsigned_tag; };

// zero trait
template<typename T>
struct ZeroTrait<T, typename std::enable_if<
    CTraits<T>::is_basic &&
    !CTraits<T>::is_string
    >::type>
{ using tag = default_tag; };

template<typename T>
struct ZeroTrait<T, typename std::enable_if<CTraits<T>::is_string>::type>
{ using tag = string_tag; };

// name trait
template<typename T>
struct NameTrait<T, typename std::enable_if<
    CTraits<T>::is_basic &&
    !CTraits<T>::is_int &&
    !CTraits<T>::is_pointer
    >::type>
{ using tag = default_tag; };

template<typename T>
struct NameTrait<T, typename std::enable_if<CTraits<T>::is_signed_int>::type>
{ using tag = integral_tag; };

template<typename T>
struct NameTrait<T, typename std::enable_if<CTraits<T>::is_unsigned_int>::type>
{ using tag = unsigned_tag; };

template<typename T>
struct NameTrait<T, typename std::enable_if<CTraits<T>::is_pointer>::type>
{ using tag = pointer_tag; };

template<typename T>
struct LuaType<T, typename std::enable_if<CTraits<T>::is_numeric>::type>
{ static constexpr int code = LUA_TNUMBER; };

template<typename T>
struct LuaType<T, typename std::enable_if<CTraits<T>::is_pointer>::type>
{ static constexpr int code = LUA_TLIGHTUSERDATA; };

template<>
struct LuaType<bool>
{ static constexpr int code = LUA_TBOOLEAN; };

template<>
struct LuaType<std::string>
{ static constexpr int code = LUA_TSTRING; };

template<>
struct LuaType<const char*>
{ static constexpr int code = LUA_TSTRING; };

} // namespace detail

}

#endif
