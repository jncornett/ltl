#include "test_common.h"
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

namespace
{
static void execute_lua(lua_State* L, const char* s)
{
    if ( luaL_dostring(L, s) )
        FAIL( lua_tostring(L, -1) );
}

static void assert_lua(lua_State* L, std::string expr)
{
    std::string lua_s = "assert(" + expr + ")";
    execute_lua(L, lua_s.c_str());
}

template<typename Class>
static Class& fetch_userdata(lua_State* L, const char* name)
{
    lua_getglobal(L, name);
    REQUIRE( lua_type(L, -1) == LUA_TUSERDATA );
    return **static_cast<Class**>(lua_touserdata(L, -1));
}

struct EventTracker
{
    std::vector<std::string> events;

    EventTracker() { }

    void add(std::string what)
    { events.push_back(what); }

    bool has(std::string what)
    {
        for ( auto event : events )
        {
            if ( event == what )
                return true;
        }

        return false;
    }
};

class UserType
{
public:
    UserType() : x(0), y(0) { }

    UserType(int t) : x { t }, y { t } { }

    UserType(int t1, int t2) : x { t1 }, y { t2 } { }

    ~UserType()
    {
        if ( events )
            events->add("dtor called");
    }

    void noop()
    { std::cout << "NOOOOOOPE" << std::endl; }

    int sum() { return x + y; }
    void set(int t1, int t2) { x = t1; y = t2; }
    bool ordered(bool reverse) { return reverse ? x >= y : x <= y; }

    EventTracker* events = nullptr;

    int x, y;
};

static int custom_ctor1(lua_State* L)
{
    int v = 4;
    const char* arg = luaL_checkstring(L, 1);
    if ( arg && !strcmp(arg, "three") )
        v = 3;

    auto handle = create_userdata<UserType>(L);
    *handle = new UserType(v);

    return 1;
}

static UserType* custom_ctor2(Ltl::Sandbox&)
{
    // FIXIT-H add sandbox methods
    return new UserType(5);
}

}

TEST_CASE( "lua userdata registration default ctor" )
{
    Vm lua(true);

    SECTION( "class and default ctor gets registered in the lua registry" )
    {
        // Pause garbage collection
        lua_gc(lua, LUA_GCSTOP, 0);

        Ltl::register_class<UserType>(lua, "UserType")
            .add_ctor<>();

        assert_lua(lua, "UserType");
        execute_lua(lua, "ut = UserType.new()");
        assert_lua(lua, "ut");

        // Acquire the userdata
        auto& ut = fetch_userdata<UserType>(lua, "ut");

        lua_settop(lua, 0);

        SECTION( "default ctor gets called" )
        {
            CHECK( ut.x == 0 );
            CHECK( ut.y == 0 );
        }

        SECTION( "dtor gets called" )
        {
            EventTracker events;
            REQUIRE( !events.has("dtor called") );

            // now attach the state object to it
            ut.events = &events;

            // remove all references to ut and force garbage collection
            lua_pushnil(lua);
            lua_setglobal(lua, "ut");
            lua_settop(lua, 0);

            lua_gc(lua, LUA_GCCOLLECT, 0);

            CHECK( events.has("dtor called") );
        }
    }
}

TEST_CASE( "lua userdata registration non-default ctors" )
{
    Vm lua(true);

    SECTION( "ctor<int>" )
    {
        Ltl::register_class<UserType>(lua, "UserType")
            .add_ctor<int>();

        execute_lua(lua, "ut = UserType.new(1)");

        auto& ut = fetch_userdata<UserType>(lua, "ut");

        CHECK( ut.x == 1 );
        CHECK( ut.y == 1 );
    }

    SECTION( "ctor<int, int>" )
    {
        Ltl::register_class<UserType>(lua, "UserType")
            .add_ctor<int, int>();

        execute_lua(lua, "ut = UserType.new(2, 4)");

        auto& ut = fetch_userdata<UserType>(lua, "ut");

        CHECK( ut.x == 2 );
        CHECK( ut.y == 4 );
    }
}

TEST_CASE( "lua userdata registration with custom ctor" )
{
    Vm lua(true);

    SECTION( "lua_CFunction signature" )
    {
        SECTION( "static function" )
        {
            Ltl::register_class<UserType>(lua, "UserType")
                .add_ctor(custom_ctor1);
        }

        SECTION( "lambda" )
        {
            Ltl::register_class<UserType>(lua, "UserType")
                .add_ctor([](lua_State* L) {
                    return custom_ctor1(L);
                });
        }

        execute_lua(lua, "ut = UserType.new(\"three\")");

        auto& ut = fetch_userdata<UserType>(lua, "ut");

        CHECK( ut.x == 3 );
        CHECK( ut.y == 3 );
    }

    // SECTION( "convenience signature" )
    // {
    //     SECTION( "static function" )
    //     {
    //         Ltl::register_class<UserType>(lua, "UserType")
    //             .add_ctor(custom_ctor2);
    //     }

    //     SECTION( "lambda" )
    //     {
    //         Ltl::register_class<UserType>(lua, "UserType")
    //             .add_ctor([](Ltl::Sandbox& sb) {
    //                 return custom_ctor2(sb);
    //             });
    //     }

    //     execute_lua(lua, "ut = UserType.new(\"five\")");

    //     auto& ut = fetch_userdata<UserType>(lua, "ut");

    //     CHECK( ut.x == 5 );
    //     CHECK( ut.y == 5 );
    // }
}
