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

namespace detail
{
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

enum class PropertyAccess
{ READ_WRITE, READ_ONLY, WRITE_ONLY };

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
        detail::AutoCtorHelper<Class, Pack...>::push(L, methods);
        return *this;
    }

    template<typename F>
    ClassRegistrar& add_ctor(F&& fn)
    {
        detail::CustomCtorHelper<Class, F>::push(L, methods, fn);
        return *this;
    }

    template<typename F>
    ClassRegistrar& add_dtor(F&&)
    { return *this; }

    template<typename F>
    ClassRegistrar& add_function(std::string, F&&)
    { return *this; }

    template<typename F>
    ClassRegistrar& add_static_function(std::string, F&&)
    { return *this; }

private:
    void add_default_dtor()
    { detail::AutoDtorHelper<Class>::push(L, meta); }

    void open()
    {
        Userdata<Class>::set_type_name(name);
        methods = detail::new_lib(L, name);
        meta = detail::new_metalib(L, name);
        add_default_dtor();
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

    lua_State* L;
    std::string name;
    bool closed;
    int methods, meta;
};

class LibRegistrar
{
public:
    LibRegistrar(lua_State* L_, std::string n) :
        L { L_ }, name { n }
    { open(); }

    ~LibRegistrar()
    {
        if ( !closed )
        {
            closed = true;
            close();
        }
    }

    template<typename F>
    LibRegistrar& add_function(std::string, F&&)
    { return *this; }

private:
    void open();
    void close();

    lua_State* L;
    std::string name;
    bool closed = false;
};



template<typename T>
ClassRegistrar<T> register_class(lua_State* L, std::string name)
{ return ClassRegistrar<T>(L, name); }

} // namespace Ltl

#endif
