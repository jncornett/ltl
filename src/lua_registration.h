#ifndef LUA_REGISTRATION_H
#define LUA_REGISTRATION_H

#include <cassert>
#include <iostream>
#include <utility>
#include <string>
#include <luajit-2.0/lua.hpp>

#include "lua_userdata.h"
#include "lua_registration_helpers.h"

namespace Ltl
{

class Helper { };

namespace detail
{
template<typename Class, typename F>
struct FunctionRegistrationHelper { };

// Proper member function pointer specialization
template<typename Class, typename Ret, typename... Args>
struct FunctionRegistrationHelper<Class, Ret(Class::*)(Args...)>
{
    template<typename Wrapper>
    static int proxy(lua_State*)
    { return 0; }

    template<typename F>
    static void push(lua_State* L, std::string name, int table, F&& fn)
    {
        std::cout << "MEM fn push called\n";
        using wrapper_t = decltype(std::mem_fn(fn));

        lua_pushstring(L, name.c_str());

        auto mf_upvalue = reinterpret_cast<wrapper_t*>(
            lua_newuserdata(L, sizeof(wrapper_t))
        );

        assert(mf_upvalue);

        lua_pushcclosure(L, &proxy<wrapper_t>, 1);
        lua_rawset(L, table);
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
        detail::FunctionRegistrationHelper<Class, F>::push(
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
