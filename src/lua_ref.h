#ifndef LUA_REF_H
#define LUA_REF_H

#include "lua_stack_api.h"
#include "lua_util.h"

struct lua_State;

namespace Ltl
{

namespace detail
{

template<typename Subclass = void>
class Ref;

// FIXIT-M need more descriptive tag names + need to not use tags for multiple
// policies unless the semantics are identical
struct generic_tag {};
struct ref_tag {};

template<>
struct PushPolicy<ref_tag>
{
    template<typename T>
    static void push(lua_State* L, const T& v)
    { lua_pushvalue(L, v.index()); }
};

template<>
struct CastPolicy<ref_tag>
{
    template<typename T>
    static T cast(lua_State* L, int n)
    { return { L, util::abs_index(L, n) }; }
};

template<>
struct TypePolicy<generic_tag>
{
    // Ref represents all types, including LUA_TNONE
    template<typename T>
    static bool type(lua_State*, int)
    { return true; }
};

template<>
struct ZeroPolicy<ref_tag>
{
    template<typename T>
    static T zero(lua_State*, int)
    { return { nullptr, 0 }; }
};

template<>
struct NamePolicy<generic_tag>
{
    template<typename T>
    static std::string name(lua_State* L, int n)
    { return "Ref@" + std::to_string(util::abs_index(L, n)); }
};

// Ref itself should use the TypePolicy<generic_tag>, but we want its
// subclasses to use TypePolicy<default_tag>, so we specialize for Ref<void> here
template<>
struct TypeTrait<Ref<>>
{ using tag = generic_tag; };

template<>
struct NameTrait<Ref<>>
{ using tag = generic_tag; };

template<typename Subclass>
class Ref
{
public:
    using push_tag = ref_tag;
    using cast_tag = ref_tag;
    using type_tag = default_tag;
    using zero_tag = ref_tag;
    using name_tag = default_tag;

    // default ctor creates an invalid ref
    Ref() : Ref { nullptr, 0 } { }

    Ref(lua_State* L, int n) :
        L { L }, n { n } { }

    int index() const
    { return n; }

    virtual bool valid() const
    { return L && n && type<self_type>(L, index()); }

    // manually cause ref to be invalid
    void invalidate()
    { L = nullptr; }

    // convenience functions, only enabled on the generic ref
    template<typename T>
    bool is();

    template<typename T>
    T to();

    // convenience overload for conversion
    template<typename T>
    operator T();

protected:
    lua_State* L;
    int n;

    using self_type =
        typename std::conditional<
            std::is_void<Subclass>::value,
            Ref<void>,
            Subclass
        >::type;

    using parent_type = 
        typename std::conditional<
            std::is_void<Subclass>::value,
            void,
            Ref<Subclass>
        >::type;

};

// only enable the is() function for Ref<void>
template<>
template<typename T>
bool Ref<void>::is()
{ return type<T>(L, index()); }

// only enable to to() function for Ref<void>
template<>
template<typename T>
T Ref<void>::to()
{ return cast<T>(L, index()); }

// only enable the conversion operator for Ref<void>
template<>
template<typename T>
Ref<void>::operator T()
{
    // FIXIT-H for safety, this should either call Ltl::check(), or Ltl::get()
    // otherwise, type checking will become necessary, and the benefit of this
    // overload will be greatly reduced
    return to<T>();
}

} // namespace detail

// expose the generic Ref class only
using Ref = detail::Ref<void>;

}

#endif
