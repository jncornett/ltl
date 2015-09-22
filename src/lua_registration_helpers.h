#ifndef LUA_REGISTRATION_HELPERS_H
#define LUA_REGISTRATION_HELPERS_H

#include <iostream>
#include <luajit-2.0/lua.hpp>
#include "lua_stack_api.h"
#include "lua_userdata.h"

namespace Ltl
{

class Helper {};
namespace detail
{
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
struct CtorHelper
{
    static int proxy(lua_State* L)
    {
        std::cout << "Ctor for " << Userdata<Class>::get_type_name() << " called\n";

        auto ptr = CtorArgApplier<1, Class, Pack...>::apply(L);
        assert(ptr);

        // FIXIT-M userdata creation should go in lua_userdata?
        auto handle = reinterpret_cast<Class**>(
            lua_newuserdata(L, sizeof(Class*)));

        assert(handle);
        luaL_getmetatable(L, Userdata<Class>::get_type_name().c_str());
        assert(lua_istable(L, -1));
        lua_setmetatable(L, -2);

        *handle = ptr;

        return 1;
    }

    static void push(lua_State* L, int table)
    { push_function(L, "new", table, &proxy); }
};

template<typename Class>
struct DtorHelper
{
    static int proxy(lua_State* L)
    {
        std::cout << "Dtor for " << Userdata<Class>::get_type_name() << " called\n";
        // FIXIT-M functionality should be accessed via Userdata interface
        auto handle = reinterpret_cast<Class**>(
            luaL_checkudata(L, 1, Userdata<Class>::get_type_name().c_str())
        );

        assert(handle);

        return 0;
    }

    static void push(lua_State* L, int table)
    { push_function(L, "__gc", table, &proxy); }
};

template<typename Class, typename F>
struct FunctionRegistrationHelper { };

// Proper member function pointer specialization
template<typename Class, typename Ret, typename... Args>
struct FunctionRegistrationHelper<Class, Ret(Class::*)(Args...)>
{
    template<typename Wrapper>
    static int proxy(lua_State* L)
    {
        std::cout << "Proxy void fn called\n";
        auto& mf = *reinterpret_cast<Wrapper*>(
            lua_touserdata(L, lua_upvalueindex(1)));

        // FIXIT-H replace with function throwing a TypeError (AKA check())
        auto& cls = **reinterpret_cast<Class**>(
            luaL_checkudata(L, 1, Userdata<Class>::get_type_name().c_str()));

        try
        {
            auto ret = MemFnArgApplier<1, Args...>::template apply<Ret>(L, mf, cls);
            Ltl::push(L, ret);
        }
        catch(TypeError& e)
        {
            Ltl::push(L, e.what().c_str());
            lua_error(L);
        }

        return 1;
    }

    template<typename F>
    static void push(lua_State* L, std::string name, int table, F&& fn)
    {
        std::cout << "MEM fn push called\n";

        auto mf = std::mem_fn(fn);
        auto mf_ptr = reinterpret_cast<decltype(mf)*>(
            lua_newuserdata(L, sizeof(mf))
        );

        assert(mf_ptr);

        new (mf_ptr) decltype(mf)(mf);

        push_function(L, name.c_str(), table, &proxy<decltype(mf)>, 1);
    }
};

template<typename Class, typename... Args>
struct FunctionRegistrationHelper<Class, void(Class::*)(Args...)>
{
    template<typename Wrapper>
    static int proxy(lua_State* L)
    {
        std::cout << "Proxy void fn called\n";
        auto& mf = *reinterpret_cast<Wrapper*>(
            lua_touserdata(L, lua_upvalueindex(1)));

        auto& cls = **reinterpret_cast<Class**>(
            luaL_checkudata(L, 1, Userdata<Class>::get_type_name().c_str()));

        MemFnArgApplier<1, Args...>::template apply<void>(L, mf, cls);

        return 0;
    }

    template<typename F>
    static void push(lua_State* L, std::string name, int table, F&& fn)
    {
        std::cout << "MEM fn push called\n";

        auto mf = std::mem_fn(fn);
        auto mf_ptr = reinterpret_cast<decltype(mf)*>(
            lua_newuserdata(L, sizeof(mf))
        );

        assert(mf_ptr);

        new (mf_ptr) decltype(mf)(mf);

        push_function(L, name.c_str(), table, &proxy<decltype(mf)>, 1);
    }
};

// Raw lua C function specialization
template<typename Class>
struct FunctionRegistrationHelper<Class, int(*)(lua_State*)>
{ };

// Wrapped function specialization
template<typename Class, typename Ret>
struct FunctionRegistrationHelper<Class, Ret(*)(Helper&, Class*)>
{ };

} // namespace detail
}
#endif
