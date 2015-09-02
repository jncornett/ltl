#include "test_common.h"

TEST_CASE ( "Stack API for integral types", "[stack_api]")
{
    Vm lua;
    int v = -42;

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        CHECK( lua_tointeger(lua, -1) == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<int>(lua, -1) );
        CHECK( !Ltl::type<unsigned>(lua, -1) );
    }

    SECTION( "cast" )
    {
        CHECK( Ltl::cast<int>(lua, -1) == v );
    }

    SECTION( "zero" )
    {
        CHECK( Ltl::zero<int>(lua, 0) == 0 );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<int>(lua, 0) == "integer" );
    }
}

TEST_CASE ( "Stack API for floating point types", "[stack_api]")
{
    Vm lua;
    float v = 3.14f;

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        CHECK( lua_tonumber(lua, -1) == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<float>(lua, -1) );
    }

    SECTION( "cast" )
    {
        CHECK( Ltl::cast<float>(lua, -1) == v );
    }

    SECTION( "zero" )
    {
        CHECK( Ltl::zero<float>(lua, 0) == 0.0f );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<float>(lua, 0) == "number" );
    }
}

TEST_CASE ( "Stack API for boolean types", "[stack_api]")
{
    Vm lua;
    bool v = true;

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        CHECK( lua_toboolean(lua, -1) == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<bool>(lua, -1) );
    }

    SECTION( "cast" )
    {
        CHECK( Ltl::cast<bool>(lua, -1) == v );
    }

    SECTION( "zero" )
    {
        CHECK( Ltl::zero<bool>(lua, 0) == false );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<bool>(lua, 0) == "boolean" );
    }
}

TEST_CASE ( "Stack API for pointer types", "[stack_api]")
{
    Vm lua;
    int _x = 42;
    int *v = &_x;

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        CHECK( lua_topointer(lua, -1) == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<void*>(lua, -1) );
    }

    SECTION( "cast" )
    {
        CHECK( Ltl::cast<void*>(lua, -1) == v );
    }

    SECTION( "zero" )
    {
        CHECK( Ltl::zero<void*>(lua, 0) == nullptr );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<void*>(lua, 0) == "pointer" );
    }
}

TEST_CASE ( "Stack API for string types", "[stack_api]")
{
    Vm lua;
    std::string v("foo\0bar", 7);

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        size_t len = 0;
        std::string r(lua_tolstring(lua, -1, &len), len);
        CHECK( r == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<std::string>(lua, -1) );
    }

    SECTION( "cast" )
    {
        CHECK( Ltl::cast<std::string>(lua, -1) == v );
    }

    SECTION( "zero" )
    {
        CHECK( Ltl::zero<std::string>(lua, 0) == "" );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<std::string>(lua, 0) == "string" );
    }
}

TEST_CASE ( "Stack API for cstring types", "[stack_api]")
{
    Vm lua;
    const char* v = "foo";

    Ltl::push(lua, v);

    SECTION( "push" )
    {
        std::string r = lua_tostring(lua, -1);
        CHECK( r == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<const char*>(lua, -1) );
    }

    SECTION( "cast" )
    {
        std::string r = Ltl::cast<const char*>(lua, -1);
        CHECK( r == v );
    }

    SECTION( "zero" )
    {
        std::string r = Ltl::zero<const char*>(lua, 0);
        CHECK( r == "" );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<const char*>(lua, 0) == "string" );
    }
}
