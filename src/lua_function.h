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

    // FIXIT remove parent_type and explicitly use the base template
    // instantiation to reduce obfuscation
    Func() : parent_type() { }
    Func(lua_State* L, int n) : parent_type { L, n } { }
};

}

#endif
