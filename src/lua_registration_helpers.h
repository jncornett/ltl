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

template<int N, typename... Pack>
struct MemFnArgApplier {};

template<int N>
struct MemFnArgApplier<N>
{
    template<typename Ret, typename M, typename Class, typename... Args>
    static Ret apply(lua_State*, M mf, Class& cls, Args&&... args)
    { return mf(cls, std::forward<Args>(args)...); }
};

template<int N, typename Next, typename... Rest>
struct MemFnArgApplier<N, Next, Rest...>
{
    template<typename Ret, typename M, typename Class, typename... Args>
    static Ret apply(lua_State* L, M mf, Class& cls, Args&&... args)
    {
        return MemFnArgApplier<N+1, Rest...>::template apply<Ret>(
            L, mf, cls, std::forward<Args>(args)..., check<Next>(L, N));
    }
};


// -----------------------------------------------------------------------------
// proxy wrappers
// -----------------------------------------------------------------------------

template<typename Class, typename... Pack>
struct AutoCtorProxy
{
    static int proxy(lua_State* L)
    {
        std::cout << "Ctor for " << get_ud_type_name<Class>() << " called\n";
        auto p = CtorArgApplier<1, Class, Pack...>::apply(L);
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

template<typename Class>
struct AutoDtorProxy
{
    static int proxy(lua_State* L)
    {
        std::cout << "Dtor for " << get_ud_type_name<Class>() << " called\n";
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

template<typename Class, typename Ret, typename Enable = void>
struct AutoMemberFunctionProxy
{
    template<typename Wrapper, typename... Pack>
    static int proxy(lua_State* L)
    {
        std::cout <<
            "Auto proxy member fn on " << get_ud_type_name<Class>() << " called";

        auto& mf = *get_ud_ptr<Wrapper>(L, lua_upvalueindex(1));

        try
        {
            auto& cls = **check_ud_handle<Class>(L, 1);
            auto ret = MemFnArgApplier<1, Pack...>::template apply<Ret>(L, mf, cls);
            Ltl::push(L, ret);
        }
        catch(TypeError& e)
        {
            Ltl::push(L, e.what());
            lua_error(L);
        }

        return 1;
    }
};

template<typename Class, typename Ret>
struct AutoMemberFunctionProxy<
    Class,
    Ret,
    typename std::enable_if<std::is_void<Ret>::value>::type
>
{
    template<typename Wrapper, typename... Pack>
    static int proxy(lua_State* L)
    {
        std::cout <<
            "Auto void proxy member fn on " << get_ud_type_name<Class>() << " called";

        auto& mf = *get_ud_ptr<Wrapper>(L, lua_upvalueindex(1));

        try
        {
            auto& cls = **check_ud_handle<Class>(L, 1);
            MemFnArgApplier<1, Pack...>::template apply<Ret>(L, mf, cls);
        }
        catch(TypeError& e)
        {
            Ltl::push(L, e.what());
            lua_error(L);
        }

        return 0;
    }
};

template<typename Class, typename F>
struct FunctionRegistrationHelper {};

// // Proper member function pointer specialization
template<typename Class, typename Ret, typename... Args>
struct FunctionRegistrationHelper<Class, Ret(Class::*)(Args...)>
{
    template<typename F>
    static void push(lua_State* L, std::string name, int table, F&& fn)
    {
        auto mf = std::mem_fn(fn);
        auto mf_ptr = alloc_ud_ptr<decltype(mf)>(L);
        assert(mf_ptr);

        // copy construct member function wrapper in the Lua VM
        new (mf_ptr) decltype(mf)(mf);

        push_function(L, name.c_str(), table,
            &AutoMemberFunctionProxy<Class, Ret>::template proxy<decltype(mf), Args...>);
    }
};

// // Raw lua C function specialization
// template<typename Class>
// struct FunctionRegistrationHelper<Class, int(*)(lua_State*)>
// { };

} // namespace detail
}
#endif
