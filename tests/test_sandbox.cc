#include "test_common.h"
#include <iostream>

namespace
{
class RegisteredType
{
public:
    void a_method() { }
};

class UnregisteredType {};
}

static int implicit_conversion_test(
    RegisteredType& rt_ref, RegisteredType* rt_ptr, int v)
{
    rt_ref.a_method();
    rt_ptr->a_method();
    return v;
}

TEST_CASE ( "check core", "[sandbox]" )
{
    Vm lua;
    int v = -42;

    lua_pushinteger(lua, v);

    CHECK_NOTHROW( Ltl::check<int>(lua, 1) );
    CHECK_THROWS_AS( Ltl::check<const char*>(lua, 1), Ltl::TypeError );
    CHECK_THROWS_AS( Ltl::check<int>(lua, lua_gettop(lua) + 1), Ltl::TypeError );
}

// FIXIT-L should this go with the other userdata tests?
TEST_CASE ( "check userdata", "[sandbox]" )
{
    Vm lua;

    SECTION( "unregistered class" )
    {
        allocate_userdata<UnregisteredType>(lua);
        CHECK_THROWS_AS( Ltl::check<UnregisteredType>(lua, 1), Ltl::TypeError );
    }

    SECTION( "registered class" )
    {
        PointerManager<RegisteredType> pm;
        register_userdata<RegisteredType>(lua, "RegisteredType");
        auto handle = create_userdata<RegisteredType>(lua);
        *handle = pm;

        CHECK_NOTHROW( Ltl::check<RegisteredType>(lua, 1) );

        RegisteredType& rt_ref = Ltl::check<RegisteredType>(lua, 1);
        rt_ref.a_method();

        RegisteredType* rt_ptr = Ltl::check<RegisteredType>(lua, 1);
        rt_ptr->a_method();
    }
}

TEST_CASE ( "implicit conversion", "[sandbox]" )
{
    Vm lua;
    PointerManager<RegisteredType> pm;
    register_userdata<RegisteredType>(lua, "RegisteredType");
    auto handle = create_userdata<RegisteredType>(lua);
    *handle = pm;

    int v = -42;
    int rv = 0;

    lua_pushinteger(lua, v);

    CHECK_NOTHROW(
        rv = implicit_conversion_test(
            Ltl::check<RegisteredType>(lua, 1),
            Ltl::check<RegisteredType>(lua, 1),
            Ltl::check<int>(lua, 2)
        )
    );

    CHECK( rv == v );

    // FIXIT-M check<T&> and check<T*> not supported
}
