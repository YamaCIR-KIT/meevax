#ifndef INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP
#define INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP

#include <boost/math/constants/constants.hpp>

#include <meevax/kernel/complex.hpp>
#include <meevax/kernel/exact_integer.hpp>
#include <meevax/kernel/floating_point.hpp>
#include <meevax/kernel/ratio.hpp>

#include <typeindex>

namespace meevax { inline namespace kernel
{
  /* ---- Numbers --------------------------------------------------------------
   *
   *  number
   *   `-- complex
   *        `-- real
   *             |-- floating-point (IEEE 754)
   *             |    |-- binary  16
   *             |    |-- binary  32 (C++ single float)      = floating_point<float>
   *             |    |-- binary  64 (C++ double float)      = floating_point<double>
   *             |    |-- binary  80 (C++ long double float) = floating_point<long double>
   *             |    `-- binary 128
   *             `-- rational
   *                  |-- ratio
   *                  `-- exact-integer
   *                       |-- multi-precision exact-integer
   *                       `-- fixed precision exact-integer
   *                            |-- signed and unsigned   8  = number<std::u?int8_t>
   *                            |-- signed and unsigned  16  = number<std::u?int16_t>
   *                            |-- signed and unsigned  32  = number<std::u?int32_t>
   *                            |-- signed and unsigned  64  = number<std::u?int64_t>
   *                            `-- signed and unsigned 128  = number<std::u?int128_t>
   *
   * ------------------------------------------------------------------------ */

  /* ---- Multi-Precision Exact-Integer ------------------------------------- */

  inline auto exact_integer::as_inexact() const
  {
    return floating_point(value.convert_to<default_float::value_type>());
  }

  /* ---- Ratio ------------------------------------------------------------- */

  auto ratio::is_integer() const
  {
    return denominator().as<exact_integer>().is(1);
  }

  auto ratio::as_inexact() const
  {
    return floating_point(numerator().as<exact_integer>().as_inexact() / denominator().as<exact_integer>().as_inexact());
  }

  auto ratio::reduce() -> const ratio&
  {
    if (const exact_integer divisor {
          boost::multiprecision::gcd(
            numerator().as<exact_integer>().value,
            denominator().as<exact_integer>().value)
        };
        not divisor.is(1))
    {
      numerator() = make(numerator().as<exact_integer>() / divisor);
      denominator() = make(denominator().as<exact_integer>() / divisor);
    }

    return *this;
  }

  /* ---- Floating-Point Numbers -------------------------------------------- */

  template <typename T>
  inline auto floating_point<T>::as_exact() const
  {
    return static_cast<exact_integer>(value);
  }

  template <typename T>
  inline constexpr auto floating_point<T>::as_inexact() const noexcept
  {
    return default_float(*this); // XXX ???
  }

  /* ---- Generic ----------------------------------------------------------- */

  auto exact = [](const object& z)
  {
    #define BOILERPLATE(TYPE)                                                  \
    {                                                                          \
      typeid(TYPE), [](auto&& z)                                               \
      {                                                                        \
        return z.template as<TYPE>().as_exact();                               \
      }                                                                        \
    }

    static const std::unordered_map<
      std::type_index,
      std::function<exact_integer (const object&)>
    >
    match
    {
      BOILERPLATE(single_float),
      BOILERPLATE(double_float),
      // BOILERPLATE(ratio),
      BOILERPLATE(exact_integer),
    };

    #undef BOILERPLATE

    return match.at(z.type())(z);
  };

  auto inexact = [](const object& z)
  {
    #define BOILERPLATE(TYPE)                                                  \
    {                                                                          \
      typeid(TYPE), [](auto&& z)                                               \
      {                                                                        \
        return z.template as<TYPE>().as_inexact();                             \
      }                                                                        \
    }

    static const std::unordered_map<
      std::type_index,
      std::function<default_float (const object&)>
    >
    match
    {
      BOILERPLATE(single_float),
      BOILERPLATE(double_float),
      BOILERPLATE(ratio),
      BOILERPLATE(exact_integer),
    };

    #undef BOILERPLATE

    return match.at(z.type())(z);
  };

  auto is_nan = [](const object& x)
  {
    if (x.is<null>())
    {
      return false;
    }
    else if (x.is<single_float>())
    {
      return std::isnan(x.as<single_float>());
    }
    else if (x.is<double_float>())
    {
      return std::isnan(x.as<double_float>());
    }
    else
    {
     return false;
    }
  };

  /* ---- Arithmetic Operations ------------------------------------------------
   *
   * ┌─────────┬─────────┬─────────┬─────────┬───────┬─────────┬
   * │ LHS\RHS │   f32   │   f64   │   big   │ ratio │ complex │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   f32   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   f64   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   big   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │  ratio  │    f    │    f    │    f    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │ complex │    f    │    f    │    f    │   f   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   *
   * ------------------------------------------------------------------------ */

  /* ---- Multi-Precision Exact-Integer ------------------------------------- */

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const exact_integer& lhs, const floating_point<T>& rhs) \
  {                                                                            \
    return floating_point(lhs.as_inexact() SYMBOL rhs);                        \
  } static_assert(true)

  BOILERPLATE(*);
  BOILERPLATE(+);
  BOILERPLATE(-);
  BOILERPLATE(/);
  BOILERPLATE(%);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  auto operator SYMBOL(const exact_integer& lhs, const ratio& rhs)             \
  {                                                                            \
    return ratio(lhs * rhs.denominator() SYMBOL rhs.numerator(), rhs.denominator()); \
  } static_assert(true)

  BOILERPLATE(+);
  BOILERPLATE(-);

  #undef BOILERPLATE

  auto operator *(const exact_integer& lhs, const ratio& rhs)
  {
    return ratio(lhs * rhs.numerator(), rhs.denominator());
  }

  auto operator /(const exact_integer& lhs, const ratio& rhs)
  {
    return lhs * rhs.invert();
  }

  // TODO
  auto operator %(const exact_integer&, const ratio& rhs)
  {
    return rhs;
  }

  /* ---- Ratio ------------------------------------------------------------- */


  /* ---- Floating-Point Numbers -------------------------------------------- */

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const floating_point<T>& lhs, const exact_integer& rhs) \
  {                                                                            \
    return floating_point(lhs SYMBOL rhs.as_inexact());                        \
  } static_assert(true)

  BOILERPLATE(*);
  BOILERPLATE(+);
  BOILERPLATE(-);
  BOILERPLATE(/);
  BOILERPLATE(%);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const floating_point<T>& lhs, const ratio& rhs)         \
  {                                                                            \
    return floating_point(lhs SYMBOL rhs.as_inexact());                        \
  } static_assert(true)

  BOILERPLATE(*);
  BOILERPLATE(+);
  BOILERPLATE(-);
  BOILERPLATE(/);
  BOILERPLATE(%);

  #undef BOILERPLATE

  /* ---- Arithmetic Operation Dispatcher --------------------------------------
   *
   *
   * ------------------------------------------------------------------------ */

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  let operator SYMBOL(const floating_point<T>& lhs, const object& rhs)         \
  {                                                                            \
    if (rhs)                                                                   \
    {                                                                          \
      if (rhs.is<ratio>())                                                     \
      {                                                                        \
        return make(lhs SYMBOL rhs.as<ratio>());                               \
      }                                                                        \
      if (rhs.is<exact_integer>())                                             \
      {                                                                        \
        return make(lhs SYMBOL rhs.as<exact_integer>());                       \
      }                                                                        \
      else if (rhs.is<single_float>())                                         \
      {                                                                        \
        return make(lhs SYMBOL rhs.as<single_float>());                        \
      }                                                                        \
      else if (rhs.is<double_float>())                                         \
      {                                                                        \
        return make(lhs SYMBOL rhs.as<double_float>());                        \
      }                                                                        \
    }                                                                          \
                                                                               \
    throw error("no viable operation '" #SYMBOL " with ", lhs, " and ", rhs);  \
  } static_assert(true)

  BOILERPLATE(*);
  BOILERPLATE(+);
  BOILERPLATE(-);
  BOILERPLATE(/);
  BOILERPLATE(%);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  let operator SYMBOL(const exact_integer& lhs, const object& rhs)             \
  {                                                                            \
    static const std::unordered_map<                                           \
      std::type_index,                                                         \
      std::function<object (const exact_integer&, const object&)>              \
    >                                                                          \
    overloads                                                                  \
    {                                                                          \
      {                                                                        \
        typeid(ratio), [](auto&& lhs, auto&& rhs)                              \
        {                                                                      \
          if (auto result { lhs SYMBOL rhs.template as<ratio>() }; result.reduce().is_integer()) \
          {                                                                    \
            return result.numerator();                                         \
          }                                                                    \
          else                                                                 \
          {                                                                    \
            return make(result);                                               \
          }                                                                    \
        }                                                                      \
      },                                                                       \
      {                                                                        \
        typeid(exact_integer), [](auto&& lhs, auto&& rhs)                      \
        {                                                                      \
          return make(lhs SYMBOL rhs.template as<exact_integer>());            \
        }                                                                      \
      },                                                                       \
      {                                                                        \
        typeid(single_float), [](auto&& lhs, auto&& rhs)                       \
        {                                                                      \
          return make(lhs SYMBOL rhs.template as<single_float>());             \
        }                                                                      \
      },                                                                       \
      {                                                                        \
        typeid(double_float), [](auto&& lhs, auto&& rhs)                       \
        {                                                                      \
          return make(lhs SYMBOL rhs.template as<double_float>());             \
        }                                                                      \
      },                                                                       \
    };                                                                         \
                                                                               \
    if (auto iter { overloads.find(rhs.type()) }; iter != std::end(overloads)) \
    {                                                                          \
      return std::invoke(cdr(*iter), lhs, rhs);                                \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      throw error("no viable operation '" #SYMBOL " with ", lhs, " and ", rhs); \
    }                                                                          \
  } static_assert(true)

  BOILERPLATE(*);
  BOILERPLATE(+);
  BOILERPLATE(-);
  BOILERPLATE(/);
  BOILERPLATE(%);

  #undef BOILERPLATE

  /* ---- Arithmetic Comparisons -----------------------------------------------
   *
   * TODO CONSIDER EPSILON
   *
   * ┌─────────┬─────────┬─────────┬─────────┬───────┬─────────┬
   * │ LHS\RHS │   f32   │   f64   │   big   │ ratio │ complex │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   f32   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   f64   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │   big   │    t    │    t    │    t    │   t   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │  ratio  │    f    │    f    │    f    │   f   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   * │ complex │    f    │    f    │    f    │   f   │    f    │
   * ├─────────┼─────────┼─────────┼─────────┼───────┼─────────┼
   *
   * ------------------------------------------------------------------------ */

  /* ---- Multi-Precision Exact-Integer --------------------------------------*/

  #define BOILERPLATE(SYMBOL)                                                  \
  auto operator SYMBOL(const exact_integer& lhs, ratio& rhs)                   \
  {                                                                            \
    if (rhs.reduce().is_integer())                                             \
    {                                                                          \
      return lhs SYMBOL rhs.numerator();                                       \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      return false;                                                            \
    }                                                                          \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const exact_integer& lhs, const floating_point<T>& rhs) \
  {                                                                            \
    return lhs.value.convert_to<T>() SYMBOL rhs.value;                         \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  /* ---- Ratio ------------------------------------------------------------- */

  #define BOILERPLATE(SYMBOL)                                                  \
  auto operator SYMBOL(const ratio& lhs, const exact_integer& rhs)             \
  {                                                                            \
    auto copy { lhs };                                                         \
                                                                               \
    if (copy.reduce().is_integer())                                            \
    {                                                                          \
      return copy.numerator().as<exact_integer>() SYMBOL rhs;                  \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      return false;                                                            \
    }                                                                          \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const ratio& lhs, const floating_point<T>& rhs)         \
  {                                                                            \
    return lhs.as_inexact() SYMBOL rhs;                                        \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  /* ---- Floating-Point Number --------------------------------------------- */

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const floating_point<T>& lhs, const exact_integer& rhs) \
  {                                                                            \
    return lhs.value SYMBOL rhs.value.convert_to<T>();                         \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T>                                                        \
  auto operator SYMBOL(const floating_point<T>& lhs, const ratio& rhs)         \
  {                                                                            \
    return lhs SYMBOL rhs.as_inexact();                                        \
  } static_assert(true)

  BOILERPLATE(!=);
  BOILERPLATE(<);
  BOILERPLATE(<=);
  BOILERPLATE(==);
  BOILERPLATE(>);
  BOILERPLATE(>=);

  #undef BOILERPLATE

  /* ---- Arithmetic Comparison Dispatcher ---------------------------------- */

  #define BOILERPLATE(NUMBER, SYMBOL)                                          \
  auto operator SYMBOL(const NUMBER& lhs, const object& rhs) -> bool           \
  {                                                                            \
    if (rhs)                                                                   \
    {                                                                          \
      if (rhs.is<exact_integer>())                                             \
      {                                                                        \
        return lhs SYMBOL rhs.as<exact_integer>();                             \
      }                                                                        \
      else if (rhs.is<ratio>())                                                \
      {                                                                        \
        return lhs SYMBOL rhs.as<ratio>();                                     \
      }                                                                        \
      else if (rhs.is<single_float>())                                         \
      {                                                                        \
        return lhs SYMBOL rhs.as<single_float>();                              \
      }                                                                        \
      else if (rhs.is<double_float>())                                         \
      {                                                                        \
        return lhs SYMBOL rhs.as<double_float>();                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    throw error("no viable operation '" #SYMBOL " with ", lhs, " and ", rhs);  \
  } static_assert(true)

  template <typename T> BOILERPLATE(floating_point<T>, !=);
  template <typename T> BOILERPLATE(floating_point<T>, < );
  template <typename T> BOILERPLATE(floating_point<T>, <=);
  template <typename T> BOILERPLATE(floating_point<T>, ==);
  template <typename T> BOILERPLATE(floating_point<T>, > );
  template <typename T> BOILERPLATE(floating_point<T>, >=);

  BOILERPLATE(exact_integer, !=);
  BOILERPLATE(exact_integer, < );
  BOILERPLATE(exact_integer, <=);
  BOILERPLATE(exact_integer, ==);
  BOILERPLATE(exact_integer, > );
  BOILERPLATE(exact_integer, >=);

  BOILERPLATE(ratio, !=);
  BOILERPLATE(ratio, < );
  BOILERPLATE(ratio, <=);
  BOILERPLATE(ratio, ==);
  BOILERPLATE(ratio, > );
  BOILERPLATE(ratio, >=);

  #undef BOILERPLATE
}} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP