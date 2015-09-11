#ifndef LUA_FUNC_H
#define LUA_FUNC_H

#include <luajit-2.0/lua.hpp>
#include "lua_ref.h"

namespace Ltl
{

class Func : public detail::Ref<Func>
{
public:
    static constexpr int lua_type_code = LUA_TFUNCTION;

    Func() : detail::Ref<Func>() { }
    Func(lua_State* L, int n) : detail::Ref<Func> { L, n } { }
};

}

#endif
