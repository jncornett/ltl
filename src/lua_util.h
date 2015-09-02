#ifndef LUA_UTIL_H
#define LUA_UTIL_H

#include <luajit-2.0/lua.hpp>

namespace Ltl
{

namespace util
{

static inline int abs_index(int n, int bottom, int top)
{
    if ( n < 0 )
        n += top + 1;
    else if ( n > 0 )
        n += bottom - 1;

    return ( n > 0 ) ? n : 0;
}

static inline int abs_index(int n, int top)
{ return abs_index(n, 1, top); }

static inline int abs_index(lua_State* L, int n)
{ return abs_index(n, lua_gettop(L)); }

} // namespace util

}

#endif
