#ifndef LUA_REGISTRATION_HELPERS_H
#define LUA_REGISTRATION_HELPERS_H

#include <iostream>
#include <string>
#include <luajit-2.0/lua.hpp>
#include "lua_stack_api.h"
#include "lua_userdata.h"
#include "lua_sandbox.h"

namespace Ltl
{

namespace detail
{

template<typename T>
static inline std::string get_ud_type_name()
{ return Userdata<T>::get_type_name(); }

template<typename T>
static inline T** check_ud_handle(lua_State* L, int n)
{
    return reinterpret_cast<T**>(
        luaL_checkudata(L, n, get_ud_type_name<T>().c_str())
    );
}

template<typename T>
static inline T** get_ud_handle(lua_State* L, int n)
{ return reinterpret_cast<T**>(lua_touserdata(L, n)); }


template<typename T>
static inline T* check_ud_ptr(lua_State* L, int n)
{
    return reinterpret_cast<T*>(
        luaL_checkudata(L, n, get_ud_handle<T>().c_str())
    );
}

template<typename T>
static inline T* get_ud_ptr(lua_State* L, int n)
{ return reinterpret_cast<T*>(lua_touserdata(L, n)); }

template<typename T>
static inline T** alloc_ud_handle(lua_State* L)
{ return reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*))); }

template<typename T>
static inline T* alloc_ud_ptr(lua_State* L)
{ return reinterpret_cast<T*>(lua_newuserdata(L, sizeof(T))); }

// -----------------------------------------------------------------------------
// C function stack API extension
// -----------------------------------------------------------------------------
struct cfunction_tag {};

template<>
struct PushTrait<lua_CFunction>
{ using tag = cfunction_tag; };

template<>
struct PushPolicy<cfunction_tag>
{
    template<typename T>
    static void push(lua_State* L, T v, int nup = 0)
    { lua_pushcclosure(L, v, nup); }
};

template<typename F>
static inline void push_function(
    lua_State* L, std::string name, int table, F&& fn, int nup = 0)
{
    push(L, fn, nup);
    push(L, name);

    // reverse the order of name->fn to prepare for rawset
    lua_insert(L, lua_gettop(L) - 1);
    lua_rawset(L, table);
}

// -----------------------------------------------------------------------------
// argument appliers
// -----------------------------------------------------------------------------

template<int N, typename Ret, typename... Pack>
struct ArgumentApplier {};

template<int N, typename Ret>
struct ArgumentApplier<N, Ret>
{
    template<typename F, typename... Args>
    static Ret apply(lua_State*, F fn, Args&&... args)
    { return fn(std::forward<Args>(args)...); }
};

template<int N, typename Ret, typename Next, typename... Rest>
struct ArgumentApplier<N, Ret, Next, Rest...>
{
    template<typename F, typename... Args>
    static Ret apply(lua_State* L, F fn, Args&&... args)
    {
        return ArgumentApplier<N+1, Ret, Rest...>::apply(L, fn,
            std::forward<Args>(args)..., check<Next>(L, N));
    }
};

// Need a separate implementation for operator new because of the slightly
// different syntax.

template<int N, typename Class, typename... Pack>
struct NewArgumentApplier {};

template<int N, typename Class>
struct NewArgumentApplier<N, Class>
{
    template<typename... Args>
    static Class* apply(lua_State*, Args&&... args)
    { return new Class(std::forward<Args>(args)...); }
};

template<int N, typename Class, typename Next, typename... Rest>
struct NewArgumentApplier<N, Class, Next, Rest...>
{
    template<typename... Args>
    static Class* apply(lua_State* L, Args&&... args)
    {
        return NewArgumentApplier<N+1, Class, Rest...>::apply(L,
            std::forward<Args>(args)..., check<Next>(L, N));
    }
};


template<int N, typename Class, typename... Pack>
struct CtorArgApplier {};

template<int N, typename Class>
struct CtorArgApplier<N, Class>
{
    template<typename... Args>
    static Class* apply(lua_State*, Args&&... args)
    { return new Class(std::forward<Args>(args)...); }
};

template<int N, typename Class, typename Next, typename... Rest>
struct CtorArgApplier<N, Class, Next, Rest...>
{
    template<typename... Args>
    static Class* apply(lua_State* L, Args&&... args)
    {
        return CtorArgApplier<N+1, Class, Rest...>::apply(
            L, std::forward<Args>(args)..., check<Next>(L, N));
    }
};

// -----------------------------------------------------------------------------
// basic garbage-collected C++ object
// -----------------------------------------------------------------------------

// construct a generic garbage-collected C++ object in the lua VM
template<typename T>
struct GCObject
{
    static int dtor(lua_State* L)
    {
        // FIXIT-H add some error checking
        auto handle = get_ud_handle<T>(L, 1);
        if ( handle && *handle )
        {
            delete *handle;
            *handle = nullptr;
        }

        return 0;
    }

    static T** create(lua_State* L)
    {
        // FIXIT-H add some error checking

        // Create a new userdata
        auto handle = alloc_ud_handle<T>(L);
        int ud = lua_gettop(L);

        assert(handle);
        *handle = nullptr;

        // Create a metatable
        lua_newtable(L);
        int meta = lua_gettop(L);
        push_function(L, "__gc", meta, &dtor);

        // Set the metatable for the object
        lua_setmetatable(L, ud);

        return handle;
    }

    static T** get(lua_State* L, int n)
    {
        // FIXIT-H add some error checking
        return get_ud_handle<T>(L, n);
    }
};

// -----------------------------------------------------------------------------
// proxy wrappers
// -----------------------------------------------------------------------------

using raw_fn_t = lua_CFunction;
using raw_functor_t = std::function<int(lua_State*)>;

template<typename Class>
using wrapped_ctor_fn_t = Class*(*)(Sandbox&);

template<typename Class>
using wrapped_ctor_functor_t = std::function<Class*(Sandbox&)>;

template<typename Class, typename... Pack>
struct AutoCtorProxy
{
    static int proxy(lua_State* L)
    {
        auto p = NewArgumentApplier<1, Class, Pack...>::apply(L);
        assert(p);

        auto h = alloc_ud_handle<Class>(L);
        assert(h);

        *h = p;

        luaL_getmetatable(L, get_ud_type_name<Class>().c_str());
        assert(lua_istable(L, -1));
        lua_setmetatable(L, -2);

        return 1;
    }
};

template<typename Class, typename... Pack>
struct AutoCtorHelper
{
    static void push(lua_State* L, int table)
    { push_function(L, "new", table, &AutoCtorProxy<Class, Pack...>::proxy); }
};

template<typename Class, typename F>
struct CustomCtorProxy {};

template<typename Class>
struct CustomCtorProxy<Class, wrapped_ctor_fn_t<Class>>
{
    static int proxy(lua_State* L)
    {
        auto fn = static_cast<wrapped_ctor_fn_t<Class>>(
            const_cast<void*>(lua_topointer(L, lua_upvalueindex(1)))
        );

        return 0;
    }
};

template<typename Class>
struct CustomCtorProxy<Class, raw_functor_t>
{
    static int proxy(lua_State* L)
    {
        auto& fn = **get_ud_handle<raw_functor_t>(L, lua_upvalueindex(1));
        return fn(L);
    }
};

template<typename Class>
struct CustomCtorProxy<Class, wrapped_ctor_functor_t<Class>>
{ };

template<typename Class, typename F>
struct CustomCtorHelper
{
    static void push(lua_State* L, int table, raw_functor_t fn)
    {
        std::cout << "PUSH int(lua_State*) callable" << std::endl;

        auto handle = GCObject<raw_functor_t>::create(L);
        *handle = new raw_functor_t(fn);

        push_function(L, "new", table,
            &CustomCtorProxy<Class, raw_functor_t>::proxy, 1);
    }

    static void push(lua_State*, int, wrapped_ctor_functor_t<Class>);
};

template<typename Class>
struct CustomCtorHelper<Class, raw_fn_t>
{
    static void push(lua_State* L, int table, raw_fn_t fn)
    {
        std::cout << "PUSH lua_CFunction" << std::endl;
        push_function(L, "new", table, fn);
    }
};

template<typename Class>
struct CustomCtorHelper<Class, wrapped_ctor_fn_t<Class>>
{
    static void push(lua_State* L, int table, wrapped_ctor_fn_t<Class>)
    {
        std::cout << "PUSH wrapped function" << std::endl;
        push_function(L, "new", table,
            &CustomCtorProxy<Class, wrapped_ctor_fn_t<Class>>::proxy);
    }
};

template<typename Class>
struct AutoDtorProxy
{
    static int proxy(lua_State* L)
    {
        auto h = check_ud_handle<Class>(L, 1);
        assert(h && *h); // dtor should not be called twice

        delete *h;
        *h = nullptr;

        return 0;
    }
};

template<typename Class>
struct AutoDtorHelper
{
    static void push(lua_State* L, int table)
    { push_function(L, "__gc", table, &AutoDtorProxy<Class>::proxy); }
};

} // namespace detail
}
#endif
