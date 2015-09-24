#include "test_common.h"

namespace
{
class UnregisteredType {};
class RegisteredType
{
public:
    void a_method() { }
};
}

TEST_CASE( "Stack API for Userdata", "[stack_api][func]" )
{
    Vm lua;

    SECTION( "unregistered class" )
    {
        allocate_userdata<UnregisteredType>(lua);
        auto udata = Ltl::Userdata<UnregisteredType>(lua, lua_gettop(lua));

        SECTION( "type" )
        {
            CHECK( Ltl::type<Ltl::Userdata<UnregisteredType>>(lua, 1) == false );
        }
    }

    SECTION( "registered class" )
    {
        register_userdata<RegisteredType>(lua, "RegisteredType");
        auto handle = create_userdata<RegisteredType>(lua);
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
         allocate_userdata<UnregisteredType>(lua);
         auto udata = Ltl::Userdata<UnregisteredType>(lua, 1);
         CHECK( !udata.valid() );
     }

     SECTION( "registered type" )
     {
         PointerManager<RegisteredType> pm;

         // Not redundant because it is not guaranteed which test case will
         // execute first
         register_userdata<RegisteredType>(lua, "RegisteredType");
         auto handle = create_userdata<RegisteredType>(lua);
         *handle = pm;

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
