#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <string>
#include <luajit-2.0/lua.hpp>
#include <ltl.h>
#include "catch.hpp"

template<typename Class>
class PointerManager
{
public:
    template<typename... Args>
    PointerManager(Args&&... args)
    { ptr = new Class(std::forward<Args>(args)...); }

    ~PointerManager()
    { delete ptr; }

    operator Class*()
    { return ptr; }

private:
    Class* ptr;
};

class Vm
{
public:
    Vm(bool openlibs = false) : L { luaL_newstate() }
    {
        if ( openlibs )
            luaL_openlibs(L);
    }

    ~Vm() { lua_close(L); }

    operator lua_State*() { return L; }

private:
    lua_State* L;
};

template<typename Class>
static void register_userdata(lua_State* L, const char* name)
{
    luaL_newmetatable(L, name);
    lua_pop(L, 1);
    Ltl::Userdata<Class>::set_type_name(name);
}

template<typename Class>
static Class** allocate_userdata(lua_State* L)
{
    auto v = static_cast<Class**>(lua_newuserdata(L, sizeof(Class*)));
    REQUIRE(v);
    *v = nullptr;
    return v;
}

template<typename Class>
static void setup_userdata(lua_State* L, int n)
{
    luaL_getmetatable(L, Ltl::Userdata<Class>::get_type_name().c_str());
    REQUIRE(!lua_isnil(L, -1));
    lua_setmetatable(L, n);
}

template<typename Class>
static Class** create_userdata(lua_State* L)
{
    auto v = allocate_userdata<Class>(L);
    setup_userdata<Class>(L, lua_gettop(L));
    return v;
}


#endif
