#ifndef LUA_SANDBOX_H
#define LUA_SANDBOX_H

#include <luajit-2.0/lua.hpp>
#include "lua_exception.h"
#include "lua_userdata.h"
#include "lua_stack_core.h"
#include "lua_stack_api.h"

namespace Ltl
{

namespace detail
{

// Use the userdata tag by default
template<typename T, typename Enable = void>
struct CheckTrait
{ using tag = userdata_tag; };

template<typename T>
struct CheckTrait<T, typename std::enable_if<CTraits<T>::is_basic>::type>
{ using tag = default_tag; };

template<typename T, typename Enable = void>
struct add_userdata_wrapper
{
    using raw_t = typename std::remove_reference<
        typename std::remove_pointer<T>::type>::type;

    using type = Userdata<T>;
};

template<typename T>
struct add_userdata_wrapper<T, typename std::enable_if<CTraits<T>::is_basic>::type>
{ using type = T; };

template<typename Tag>
struct CheckPolicy
{
    template<typename T>
    static T check(lua_State* L, int n)
    {
        if ( !type<T>(L, n) )
            throw TypeError(name<T>(L, n), lua_typename(L, lua_type(L, n)));

        return cast<T>(L, n);
    }
};

template<typename T>
using userdata_wrapped_t = typename add_userdata_wrapper<T>::type;

template<>
struct CheckPolicy<userdata_tag>
{
    template<typename T>
    static userdata_wrapped_t<T> check(lua_State* L, int n)
    {
        if ( !type<userdata_wrapped_t<T>>(L, n) )
            throw TypeError(
                name<userdata_wrapped_t<T>>(L, n),
                lua_typename(L, lua_type(L, n))
            );

        return cast<userdata_wrapped_t<T>>(L, n);
    }
};

} // namespace detail

template<typename T, typename... Args>
static inline detail::userdata_wrapped_t<T> check(lua_State* L, int n)
{
    using namespace detail;
    return CheckPolicy<typename CheckTrait<T>::tag>::template check<T>(L, n);
}

class Sandbox {};

}

#endif
