#include "test_common.h"
#include <iostream>

class Foo
{
public:
    Foo(int, int) { }
    Foo(int) { }

    void a_method()
    { std::cout << "AHAHAHAHAHA" << std::endl; }

    int another_method(int x, double y)
    { return (x > y) ? 1 : 2; }
};

static void execute_lua(lua_State* L, const char* s)
{
    if ( luaL_dostring(L, s) )
        FAIL( lua_tostring(L, -1) );
}


TEST_CASE( "class registration core", "[registration]" )
{
    Vm lua(true);
    Ltl::register_class<Foo>(lua, "Foo")
        .add_ctor<int, int>()
        .add_function("a_method", &Foo::a_method);
        // .add_function("another_method", &Foo::another_method);

    execute_lua(lua, "foo = Foo.new(1, 2)");
    execute_lua(lua, "assert(foo)");
    // execute_lua(lua, "foo:a_method()");
    // execute_lua(lua, "assert( foo:another_method(5, 4) == 1 )");
}
