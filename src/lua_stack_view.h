#ifndef STACK_VIEW_H
#define STACK_VIEW_H

#include<luajit-2.0/lua.hpp>

#include "lua_ref.h"

namespace Ltl
{

class StackView
{
public:
    StackView(lua_State* L) :
        StackView(L, lua_gettop(L)) { }

    StackView(lua_State* L, int top) :
        StackView(L, 1, top) { }

    StackView(lua_State* L, int bottom, int top) :
        L { L }, bottom { bottom }, top { top } { }

    int size() const
    { return top - bottom + 1; }

    Ref operator[](int n)
    { return {L, n}; }

protected:
    lua_State* L;

public:
    const int bottom;
    const int top;
};

}

#endif
