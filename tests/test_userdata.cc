#include <cassert>
#include "test_common.h"

class UnregisteredType {};
class RegisteredType
{
public:
    void a_method() { }
};

template<typename Class>
static void register_class(lua_State* L, const char* name)
{
    luaL_newmetatable(L, name);
    lua_pop(L, 1);
    Ltl::Userdata<Class>::userdata_type_name = name;
}

template<typename Class>
static void create_userdata(lua_State* L)
{
    Class** v = static_cast<Class**>(lua_newuserdata(L, sizeof(Class*)));
    assert(v);
    *v = new Class();
    assert(*v);
    luaL_getmetatable(L, Ltl::Userdata<Class>::userdata_type_name.c_str());
    assert(!lua_isnil(L, -1));
    lua_setmetatable(L, -2);
}

TEST_CASE( "Stack API for Userdata", "[stack_api][func]" )
{
    Vm lua;

    SECTION( "unregistered class" )
    {
        void* p = lua_newuserdata(lua, sizeof(UnregisteredType*));
        auto udata = Ltl::Userdata<UnregisteredType>(lua, lua_gettop(lua));

        SECTION( "type" )
        {
            CHECK( Ltl::type<Ltl::Userdata<UnregisteredType>>(lua, 1) == false );
        }
    }

    SECTION( "registered class" )
    {
        register_class<RegisteredType>(lua, "RegisteredType");
        create_userdata<RegisteredType>(lua);
        auto udata = Ltl::Userdata<RegisteredType>(lua, lua_gettop(lua));

        SECTION( "push" )
        {
            Ltl::push(lua, udata);
            CHECK( lua_equal(lua, -1, -2) );
        }

        SECTION( "cast" )
        {
            auto u = Ltl::cast<Ltl::Userdata<RegisteredType>>(lua, -1);
            CHECK( u.index() == udata.index() );
        }

        SECTION( "type" )
        {
            CHECK( Ltl::type<Ltl::Userdata<RegisteredType>>(lua, 0) == false );
            CHECK( Ltl::type<Ltl::Userdata<RegisteredType>>(lua, 1) == true );
            CHECK( Ltl::type<Ltl::Userdata<RegisteredType>>(lua, lua_gettop(lua) + 1) == false );
        }

        SECTION( "zero" )
        {
            auto u = Ltl::zero<Ltl::Userdata<RegisteredType>>(lua, -1);
            CHECK( !u.valid() );
        }

        SECTION( "name" )
        {
            CHECK( Ltl::name<Ltl::Userdata<RegisteredType>>(lua, -1) == "Userdata<RegisteredType>" );
        }
    }

}

TEST_CASE( "Userdata members", "[userdata]" )
{
     Vm lua;

     SECTION( "unregistered type" )
     {
         void *p = lua_newuserdata(lua, sizeof(UnregisteredType*));
         REQUIRE( p );
         auto udata = Ltl::Userdata<UnregisteredType>(lua, 1);
         CHECK( !udata.valid() );
     }

     SECTION( "registered type" )
     {
         // Not redundant because it is not guaranteed which test case will
         // execute first
         register_class<RegisteredType>(lua, "RegisteredType");
         create_userdata<RegisteredType>(lua);

         auto udata = Ltl::Userdata<RegisteredType>(lua, 1);
         REQUIRE( udata.valid() );

         udata->a_method();

         RegisteredType* udata_ptr = udata;
         udata_ptr->a_method();

         RegisteredType& udata_ref = udata;
         udata_ref.a_method();

         udata.invalidate();
         CHECK( !udata.valid() );
     }
}
