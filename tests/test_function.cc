#include "test_common.h"

TEST_CASE( "Stack API for Func", "[stack_api][func]" )
{
    Vm lua;
    lua_pushcfunction(lua, [](lua_State*) { return 0; });

    auto func = Ltl::Func(lua, lua_gettop(lua));

    SECTION( "push" )
    {
        Ltl::push(lua, func);
        CHECK( lua_equal(lua, -1, -2) );
    }

    SECTION( "cast" )
    {
        auto f = Ltl::cast<Ltl::Func>(lua, -1);
        CHECK( f.index() == func.index() );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<Ltl::Func>(lua, 0) == false );
        CHECK( Ltl::type<Ltl::Func>(lua, 1) == true );
        CHECK( Ltl::type<Ltl::Func>(lua, lua_gettop(lua) + 1) == false );
    }

    SECTION( "zero" )
    {
        auto f = Ltl::zero<Ltl::Func>(lua, -1);
        CHECK( !f.valid() );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<Ltl::Func>(lua, -1) == "function" );
    }
}

TEST_CASE( "Func members", "[func]" )
{
    Vm lua;
    lua_pushcfunction(lua, [](lua_State*) { return 0; });

    auto func = Ltl::Func(lua, 1);

    CHECK( func.valid() );
    func.invalidate();
    CHECK( !func.valid() );
}
