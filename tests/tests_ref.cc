#include "test_common.h"

TEST_CASE( "Stack API for Ref", "[stack_api][ref]" )
{
    Vm lua;
    int v = 42;
    lua_pushinteger(lua, v);

    auto ref = Ltl::Ref(lua, lua_gettop(lua));

    SECTION( "push" )
    {
        Ltl::push(lua, ref);
        CHECK( lua_equal(lua, -1, -2) );
    }

    SECTION( "cast" )
    {
        auto r = Ltl::cast<Ltl::Ref>(lua, -1);
        CHECK( int(r) == v );
    }

    SECTION( "type" )
    {
        CHECK( Ltl::type<Ltl::Ref>(lua, 0) == true );
        CHECK( Ltl::type<Ltl::Ref>(lua, -1) == true );
        CHECK( Ltl::type<Ltl::Ref>(lua, lua_gettop(lua) + 1) == true );
    }

    SECTION( "zero" )
    {
        auto r = Ltl::zero<Ltl::Ref>(lua, -1);
        CHECK( !r.valid() );
    }

    SECTION( "name" )
    {
        CHECK( Ltl::name<Ltl::Ref>(lua, -1) == "Ref@1" );
    }
}

TEST_CASE( "Ref getters/setters", "[ref]" )
{
    Vm lua;
    int v = -42;
    lua_pushinteger(lua, v);

    CHECK( !Ltl::Ref(nullptr, 1).valid() );
    CHECK( !Ltl::Ref(lua, 0).valid() );

    auto ref = Ltl::Ref(lua, 1);
    CHECK( ref.valid() );

    CHECK( ref.is<int>() );
    CHECK( !ref.is<bool>() );
    CHECK( ref.to<int>() == v );
    CHECK( int(ref) == v );

    ref.invalidate();
    CHECK( !ref.valid() );
}
