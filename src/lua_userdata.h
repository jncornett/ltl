#ifndef LUA_USERDATA_H
#define LUA_USERDATA_H

#include <cassert>
#include <string>
#include <luajit-2.0/lua.hpp>
#include "lua_ref.h"

namespace Ltl
{

namespace detail
{

struct userdata_tag {};

template<>
struct TypePolicy<userdata_tag>
{
    template<typename T>
    static bool type(lua_State* L, int n)
    {
        if ( lua_type(L, n) != LUA_TUSERDATA )
            return false;

        std::string name = T::userdata_type_name;
        if ( name.empty() )
            return false;

        luaL_getmetatable(L, name.c_str());
        // userdata type is not registered in Lua
        if ( lua_isnil(L, -1) )
        {
            lua_pop(L, 1);
            return false;
        }

        lua_getmetatable(L, n);
        if ( !lua_equal(L, -1, -2) )
        {
            lua_pop(L, 2);
            return false;
        }

        lua_pop(L, 2);
        return true;
    }
};

template<>
struct NamePolicy<userdata_tag>
{
    template<typename T>
    static std::string name(lua_State*, int)
    { return "Userdata<" + T::userdata_type_name + ">"; }
};

} // namespace detail

template<typename Class>
class Userdata : public detail::Ref<Userdata<Class>>
{
public:
    using type_tag = detail::userdata_tag;
    using name_tag = detail::userdata_tag;

    static constexpr int lua_type_code = LUA_TUSERDATA;
    static std::string userdata_type_name;

    Userdata() : detail::Ref<Userdata>() { }
    Userdata(lua_State* L, int n) : detail::Ref<Userdata> { L, n } { }

    Class* operator->()
    { return get_ptr(); }

    operator Class*()
    { return get_ptr(); }

    operator Class&()
    { return *get_ptr(); }

    static void set_type_name(std::string name)
    { Userdata<Class>::userdata_type_name = name; }

    static std::string& get_type_name()
    { return Userdata<Class>::userdata_type_name; }

private:
    Class* get_ptr()
    {
        Class** p = static_cast<Class**>(lua_touserdata(this->L, this->index()));
        assert(p && *p);
        return *p;
    }
};

// FIXIT-H: will this be visible in all TUs?
// Will be set by userdata class registration
template<typename Class>
std::string Userdata<Class>::userdata_type_name = "";

}
#endif
