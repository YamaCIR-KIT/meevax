#ifndef INCLUDED_MEEVAX_SYSTEM_SRFI_1_HPP
#define INCLUDED_MEEVAX_SYSTEM_SRFI_1_HPP

#include <cassert>
#include <iterator> // std::begin, std::end, std::distance

#include <meevax/system/pair.hpp>
#include <meevax/system/exception.hpp>

// Constructors
//   - cons
//   - list
//   - xcons
//   - cons* ... variadic version of cons
//
// Predicators
//   - pair? ... objective::is<pair>
//   - null? ... objective::operator bool
//
// Selectors
//   - car ... in meevax/system/pair.hpp
//   - cdr ... in meevax/system/pair.hpp
//   - cxr ... by preprocessor macro
//   - take
//
// Miscellaneous
//   - length
//   - append
//   - zip
//
// Fold, unfold, and map
//   unimplemented (but, STL support are provided by meevax/system/cursor.hpp)
//
// Filtering & partitioning
//
// Searching
//
// Deletion
//
// Association lists
//   - assoc
//
// Set operations on lists
//   unimplemented
//
// Primitive side-effects
//   - set-car! ... objective::operator=
//   - set-cdr! ... objective::operator=
//

namespace meevax::system
{
  inline namespace constructor
  {
    template <typename T, typename U>
    objective operator|(T&& lhs, U&& rhs)
    {
      return std::make_shared<pair>(std::forward<T>(lhs), std::forward<U>(rhs));
    }

    template <typename... Ts>
    constexpr decltype(auto) cons(Ts&&... args) // is also cons*
    {
      return (args | ...);
    }

    template <typename... Ts>
    constexpr decltype(auto) list(Ts&&... args)
    {
      return (args | ... | unit);
    }

    template <typename... Ts>
    constexpr decltype(auto) xcons(Ts&&... args)
    {
      return (... | args);
    }
  } // inline namespace constructor

  inline namespace predicator
  {
  } // inline namespace predicator

  inline namespace selector
  {
    #define caar(...) car(car(__VA_ARGS__))
    #define cadr(...) car(cdr(__VA_ARGS__))
    #define cdar(...) cdr(car(__VA_ARGS__))
    #define cddr(...) cdr(cdr(__VA_ARGS__))

    #define caaar(...) car(caar(__VA_ARGS__))
    #define caadr(...) car(cadr(__VA_ARGS__))
    #define cadar(...) car(cdar(__VA_ARGS__))
    #define caddr(...) car(cddr(__VA_ARGS__))
    #define cdaar(...) cdr(caar(__VA_ARGS__))
    #define cdadr(...) cdr(cadr(__VA_ARGS__))
    #define cddar(...) cdr(cdar(__VA_ARGS__))
    #define cdddr(...) cdr(cddr(__VA_ARGS__))

    #define caaaar(...) car(caaar(__VA_ARGS__))
    #define caaadr(...) car(caadr(__VA_ARGS__))
    #define caadar(...) car(cadar(__VA_ARGS__))
    #define caaddr(...) car(caddr(__VA_ARGS__))
    #define cadaar(...) car(cdaar(__VA_ARGS__))
    #define cadadr(...) car(cdadr(__VA_ARGS__))
    #define caddar(...) car(cddar(__VA_ARGS__))
    #define cadddr(...) car(cdddr(__VA_ARGS__))
    #define cdaaar(...) cdr(caaar(__VA_ARGS__))
    #define cdaadr(...) cdr(caadr(__VA_ARGS__))
    #define cdadar(...) cdr(cadar(__VA_ARGS__))
    #define cdaddr(...) cdr(caddr(__VA_ARGS__))
    #define cddaar(...) cdr(cdaar(__VA_ARGS__))
    #define cddadr(...) cdr(cdadr(__VA_ARGS__))
    #define cdddar(...) cdr(cddar(__VA_ARGS__))
    #define cddddr(...) cdr(cdddr(__VA_ARGS__))

    objective take(const objective& exp, std::size_t size)
    {
      if (0 < size)
      {
        return car(exp) | take(cdr(exp), --size);
      }
      else
      {
        return unit;
      }
    }
  } // inline namespace selector

  inline namespace miscellaneous
  {
    template <typename E>
    decltype(auto) length(E&& e)
    {
      return std::distance(std::begin(e), std::end(e));
    }

    template <typename T, typename U>
    objective append(T&& x, U&& y)
    {
      return !x ? y : car(x) | append(cdr(x), std::forward<U>(y));
    }

    objective zip(const objective& x, const objective& y)
    {
      if (!x && !y)
      {
        return unit;
      }
      else if (x.is<pair>() && y.is<pair>())
      {
        return list(car(x), car(y)) | zip(cdr(x), cdr(y));
      }
      else
      {
        return unit;
      }
    }
  } // inline namespace miscellaneous

  inline namespace association_list
  {
    const objective& assoc(const objective& var, const objective& env)
    {
      if (!var)
      {
        return unit;
      }
      if (!env)
      {
        return unbound;
      }
      else if (caar(env) == var)
      {
        return cadar(env);
      }
      else
      {
        return assoc(var, cdr(env));
      }
    }

    objective& unsafe_assoc(const objective& var, objective& env) noexcept(false)
    {
      assert(var);

      if (!env)
      {
        throw error {var, " is unbound"};
      }
      else if (caar(env) == var)
      {
        return cadar(env);
      }
      else
      {
        return unsafe_assoc(var, cdr(env));
      }
    }
  } // inline namespace association_list

  template <typename... Ts>
  decltype(auto) display(std::ostream& os, Ts&&... args)
  {
    return (os << ... << args);
  }
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_SRFI_1_HPP
