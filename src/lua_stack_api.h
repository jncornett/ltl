#ifndef LUA_STACK_API_H
#define LUA_STACK_API_H

#include <string>
#include <utility>

#include "lua_exception.h"
#include "lua_stack_core.h"

struct lua_State;

namespace Ltl
{
// -----------------------------------------------------------------------------
// dispatchers
// -----------------------------------------------------------------------------
// FIXIT-H change API so that functions accept one metadata argument, instead of
// the current argument forwarding. This should simplify compound API functions,
// such as get() and check(). Also consider simplifying zero() so it requires no
// extra arguments.
template<typename T, typename... Args>
static inline void push(lua_State* L, T v, Args&&... args)
{
    using namespace detail;
    PushPolicy<typename PushTrait<T>::tag>::template push<T>(L,
        v, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
static inline T cast(lua_State* L, int n, Args&&... args)
{
    using namespace detail;
    return CastPolicy<typename CastTrait<T>::tag>::template cast<T>(L, n,
        std::forward<Args>(args)...);
}

template<typename T, typename... Args>
static inline bool type(lua_State* L, int n, Args&&... args)
{
    using namespace detail;
    return TypePolicy<typename TypeTrait<T>::tag>::template type<T>(L, n,
        std::forward<Args>(args)...);
}

template<typename T, typename... Args>
static inline T zero(lua_State* L, int n, Args&&... args)
{
    using namespace detail;
    return ZeroPolicy<typename ZeroTrait<T>::tag>::template zero<T>(L, n,
        std::forward<Args>(args)...);
}

template<typename T, typename... Args>
static inline std::string name(lua_State* L, int n, Args&&... args)
{
    using namespace detail;
    return NamePolicy<typename NameTrait<T>::tag>::template name<T>(L, n,
        std::forward<Args>(args)...);
}

// -----------------------------------------------------------------------------
// extended dispatchers
// -----------------------------------------------------------------------------
template<typename T, typename... Args>
static inline T get_default(lua_State* L, int n, T d, Args&&... args)
{
    if ( type<T>(L, n, std::forward<Args>(args)...) )
        return cast<T>(L, n, std::forward<Args>(args)...);
    else
        return d;
}

template<typename T, typename... Args>
static inline T get(lua_State* L, int n, Args&&... args)
{
    return get_default<T>(L, n, zero<T>(L, n, std::forward<Args>(args)...),
        std::forward<Args>(args)...);
}

// FIXIT-M implement opt() at a higher abstraction layer
// (requires too much knowledge about stack index validity)
}

#endif
