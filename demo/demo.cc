#include <iostream>

#include <luajit-2.0/lua.hpp>

#include <ltl.h>

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_close(L);

    return 0;
}
