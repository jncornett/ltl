#ifndef LUA_REGISTRATION_H
#define LUA_REGISTRATION_H

#include <cassert>
#include <iostream>
#include <utility>
#include <string>
#include <luajit-2.0/lua.hpp>

#include "lua_userdata.h"

namespace Ltl
{

class Helper
{ };

namespace detail
{
template<typename F>
static inline void push_function(lua_State* L, std::string name, F&& fn, int t)
{
    push(L, name);
    lua_pushcfunction(L, fn);
    lua_rawset(L, t);
}

// -----------------------------------------------------------------------------
// functional helpers
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

// -----------------------------------------------------------------------------
// registration helpers
// -----------------------------------------------------------------------------

template<typename Class, typename... Pack>
struct CtorHelper
{
    static int proxy(lua_State* L)
    {
        std::cout << "Ctor for " << Userdata<Class>::get_type_name() << " called\n";
        auto ptr = CtorArgApplier<1, Class, Pack...>::apply(L);
        assert(ptr);

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
    { push_function(L, "new", &proxy, table); }
};

template<typename Class>
struct DtorHelper
{
    static int proxy(lua_State* L)
    {
        std::cout << "Dtor for " << Userdata<Class>::get_type_name() << " called\n";
        auto handle = reinterpret_cast<Class**>(
            luaL_checkudata(L, 1, Userdata<Class>::get_type_name().c_str())
        );

        assert(handle);

        return 0;
    }

    static void push(lua_State* L, int table)
    { push_function(L, "__gc", &proxy, table); }
};

template<typename Class, typename F>
struct FunctionHelper { };

// Proper member function pointer specialization
template<typename Class, typename Ret, typename... Args>
struct FunctionHelper<Class, Ret(Class::*)(Args...)>
{ };

// Raw lua C function specialization
template<typename Class>
struct FunctionHelper<Class, int(*)(lua_State*)>
{ };

// Wrapped function specialization
template<typename Class, typename Ret>
struct FunctionHelper<Class, Ret(*)(Helper&, Class*)>
{ };

template<typename Class, typename Wrapper>
struct _MemberFunctionHelper
{
    template<typename F>
    static void push(lua_State* L, std::string name, int table, F&& fn)
    {
    }
};

template<typename Class, typename F>
struct MemberFunctionHelper
{
    static void push(lua_State* L, std::string name, int table, F&& fn)
    { _MemberFunctionHelper<Class, decltype(std::mem_fn(fn))>::push(L, name, table, fn); }
};

static inline int new_lib(lua_State* L, std::string libname)
{
    static const luaL_reg empty_lib[] = { { nullptr, nullptr } };
    luaL_register(L, libname.c_str(), empty_lib);
    return lua_gettop(L);
}

static inline int new_metalib(lua_State* L, std::string libname)
{
    luaL_newmetatable(L, libname.c_str());
    return lua_gettop(L);
}

} // namespace detail

template<typename Class>
class ClassRegistrar
{
public:
    ClassRegistrar(lua_State* L_, std::string n) :
        L { L_ }, name { n }, closed { false }
    { open(); }

    ~ClassRegistrar()
    {
        if ( !closed )
        {
            close();
            closed = true;
        }
    }

    template<typename... Pack>
    ClassRegistrar& add_ctor()
    {
        detail::CtorHelper<Class, Pack...>::push(L, methods);
        return *this;
    }

    template<typename F>
    ClassRegistrar& add_function(std::string name, F&& fn)
    {
        detail::MemberFunctionHelper<Class, F>::push(
            L, name, methods, std::forward<F>(fn));

        return *this;
    }

private:
    void open()
    {
        Userdata<Class>::set_type_name(name);
        methods = detail::new_lib(L, name);
        meta = detail::new_metalib(L, name);
        add_dtor();
    }

    void close()
    {
        lua_pushstring(L, "__index");
        lua_pushvalue(L, methods);
        lua_rawset(L, meta);

        lua_pushstring(L, "__metatable");
        lua_pushvalue(L, methods);
        lua_rawset(L, meta);
    }

    void add_dtor()
    { detail::DtorHelper<Class>::push(L, meta); }

    lua_State* L;
    std::string name;
    bool closed;
    int methods, meta;
};

template<typename T>
ClassRegistrar<T> register_class(lua_State* L, std::string name)
{ return ClassRegistrar<T>(L, name); }

} // namespace Ltl

#endif
