#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <string>
#include <luajit-2.0/lua.hpp>
#include <ltl.h>
#include "catch.hpp"

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

#endif
