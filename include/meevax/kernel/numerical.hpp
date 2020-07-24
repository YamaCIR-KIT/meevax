#ifndef INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP
#define INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP

#define MEEVAX_USE_MPFR
#define MEEVAX_USE_GMP

#ifdef MEEVAX_USE_MPFR
#include <boost/multiprecision/mpfr.hpp>
#else
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif

#ifdef MEEVAX_USE_GMP
#include <boost/multiprecision/gmp.hpp>
#else
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include <meevax/kernel/boolean.hpp>
#include <meevax/kernel/pair.hpp>

namespace meevax { inline namespace kernel
{
  namespace multiprecision
  {
    using namespace boost::multiprecision;

    #ifdef MEEVAX_USE_MPFR
    using real = number<mpfr_float_backend<0>, et_off>;
    #else
    using real = cpp_dec_float_100;
    #endif

    #ifdef MEEVAX_USE_GMP
    using integer = number<gmp_int, et_off>; // mpz_int
    #else
    using integer = cpp_int;
    #endif
  }

  /* ==== Numbers ==============================================================
   *
   *  number
   *   `-- complex
   *        `-- real
   *             |-- decimal (IEEE 754)
   *             |    |-- binary  16
   *             |    |-- binary  32 (C++ single float)
   *             |    |-- binary  64 (C++ double float)
   *             |    `-- binary 128
   *             `-- rational
   *                  |-- ratio
   *                  |-- arbitrary precision integer
   *                  `-- fixed precision integer
   *                       |-- signed and unsigned   8
   *                       |-- signed and unsigned  16
   *                       |-- signed and unsigned  32
   *                       |-- signed and unsigned  64
   *                       `-- signed and unsigned 128
   *
   * ======================================================================== */
  struct complex
    : public virtual pair
  {
    auto real() const noexcept -> decltype(auto) { return car(*this); }
    auto real()       noexcept -> decltype(auto) { return car(*this); }

    auto imag() const noexcept -> decltype(auto) { return cdr(*this); }
    auto imag()       noexcept -> decltype(auto) { return cdr(*this); }

    // friend auto operator +(const complex& lhs, const complex& rhs)
    // {
    //   return
    //     make<complex>(
    //       lhs.real() + rhs.real(),
    //       lhs.imag() + rhs.imag());
    // }
    //
    // template <typename T>
    // friend auto operator +(const complex& lhs, T&& rhs)
    // {
    //   return
    //     make<complex>(
    //       lhs.real() + rhs,
    //       lhs.imag());
    // }

    friend std::ostream& operator<<(std::ostream& os, const complex& z)
    {
      return os << cyan << z.real() << (0 < z.imag() ? '+' : '-') << z.imag() << "i" << reset;
    }
  };

  template <auto Bits>
  struct decimal;

  #define boilerplate(BITS, TYPE, CONVERT)                                     \
  template <>                                                                  \
  struct decimal<BITS>                                                         \
    : public std::numeric_limits<TYPE>                                         \
  {                                                                            \
    TYPE value;                                                                \
                                                                               \
    template <typename... Ts>                                                  \
    explicit constexpr decimal(Ts&&... xs)                                     \
      : value { CONVERT(std::forward<decltype(xs)>(xs)...) }                   \
    {}                                                                         \
                                                                               \
    explicit constexpr decimal(TYPE value)                                     \
      : value { value }                                                        \
    {}                                                                         \
                                                                               \
    constexpr operator TYPE() const noexcept                                   \
    {                                                                          \
      return value;                                                            \
    }                                                                          \
  }

  // XXX A terrible implementation based on optimistic assumptions.
  boilerplate(32, float, std::stof);
  boilerplate(64, double, std::stod);

  #undef boilerplate

  template <auto B>
  auto operator<<(std::ostream& os, const decimal<B>& x) -> decltype(os)
  {
    os << cyan;

    if (std::isnan(x))
    {
      return os << "+nan.0" << reset;
    }
    else if (std::isinf(x))
    {
      return os << (0 < x.value ? '+' : '-') << "inf.0" << reset;
    }
    else
    {
      return os << x.value << reset;
    }
  }

  struct [[deprecated]] real
    : public multiprecision::real
  {
    template <typename... Ts>
    explicit constexpr real(Ts&&... xs)
      : multiprecision::real { std::forward<decltype(xs)>(xs)... }
    {}

    decltype(auto) backend() const noexcept
    {
      return static_cast<const multiprecision::real&>(*this);
    }

    auto operator *(const object&) const -> object;
    auto operator +(const object&) const -> object;
    auto operator -(const object&) const -> object;
    auto operator /(const object&) const -> object;

    auto operator ==(const object&) const -> object;
    auto operator !=(const object&) const -> object;

    auto operator < (const object&) const -> object;
    auto operator <=(const object&) const -> object;
    auto operator > (const object&) const -> object;
    auto operator >=(const object&) const -> object;

    auto operator ==(const real& rhs) const { return backend() == rhs.backend(); }
    auto operator !=(const real& rhs) const { return !(*this == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const real& x)
    {
      os << cyan;

      if (const auto s { x.backend().str() }; s == "nan")
      {
        return os << "+nan.0" << reset;
      }
      else if (x.backend() == +1.0 / 0)
      {
        return os << "+inf.0" << reset;
      }
      else if (x.backend() == -1.0 / 0)
      {
        return os << "-inf.0" << reset;
      }
      else
      {
        return os << "#i" << s << reset;
      }
    }
  };

  struct rational
    : public virtual pair
  {
  };

  struct integer
    : public multiprecision::integer
  {
    template <typename... Ts>
    explicit constexpr integer(Ts&&... xs)
      : multiprecision::integer { std::forward<decltype(xs)>(xs)... }
    {}

    decltype(auto) backend() const noexcept
    {
      return static_cast<const multiprecision::integer&>(*this);
    }

    auto operator *(const object&) const -> object;
    auto operator +(const object&) const -> object;
    auto operator -(const object&) const -> object;
    auto operator /(const object&) const -> object;

    auto operator ==(const object&) const -> object;
    auto operator !=(const object&) const -> object;

    auto operator < (const object&) const -> object;
    auto operator <=(const object&) const -> object;
    auto operator > (const object&) const -> object;
    auto operator >=(const object&) const -> object;

    auto operator ==(const integer& rhs) const { return backend() == rhs.backend(); }
    auto operator !=(const integer& rhs) const { return !(*this == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const integer& x)
    {
      return os << console::cyan << x.str() << console::reset;
    }
  };

  #define DEFINE_BINARY_ARITHMETIC_REAL(SYMBOL, OPERATION)                     \
  auto real::operator SYMBOL(const object& rhs) const -> object                \
  {                                                                            \
    if (!rhs)                                                                  \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
    else if (rhs.is<real>())                                                   \
    {                                                                          \
      return make<real>(backend() SYMBOL rhs.as<multiprecision::real>());      \
    }                                                                          \
    else if (rhs.is<integer>())                                                \
    {                                                                          \
      return make<real>(backend() SYMBOL rhs.as<multiprecision::integer>());   \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
  } static_assert(true, "semicolon required after this macro")

  DEFINE_BINARY_ARITHMETIC_REAL(*, "multiplication");
  DEFINE_BINARY_ARITHMETIC_REAL(+, "addition");
  DEFINE_BINARY_ARITHMETIC_REAL(-, "subtraction");
  DEFINE_BINARY_ARITHMETIC_REAL(/, "division");

  #define DEFINE_BINARY_ARITHMETIC_INTEGER(SYMBOL, OPERATION)                  \
  auto integer::operator SYMBOL(const object& rhs) const -> object             \
  {                                                                            \
    if (!rhs)                                                                  \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
    else if (rhs.is<real>())                                                   \
    {                                                                          \
      return make<real>(backend() SYMBOL rhs.as<multiprecision::real>());      \
    }                                                                          \
    else if (rhs.is<integer>())                                                \
    {                                                                          \
      return make<integer>(backend() SYMBOL rhs.as<multiprecision::integer>());\
    }                                                                          \
    else                                                                       \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
  } static_assert(true, "semicolon required after this macro")

  DEFINE_BINARY_ARITHMETIC_INTEGER(*, "multiplication");
  DEFINE_BINARY_ARITHMETIC_INTEGER(+, "addition");
  DEFINE_BINARY_ARITHMETIC_INTEGER(-, "subtraction");
  DEFINE_BINARY_ARITHMETIC_INTEGER(/, "division");

  #define DEFINE_COMPARISON(TYPE, SYMBOL, OPERATION)                           \
  auto TYPE::operator SYMBOL(const object& rhs) const -> object                \
  {                                                                            \
    if (!rhs)                                                                  \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
    else if (rhs.is<real>())                                                   \
    {                                                                          \
      return make<boolean>(backend() SYMBOL rhs.as<multiprecision::real>());   \
    }                                                                          \
    else if (rhs.is<integer>())                                                \
    {                                                                          \
      return make<boolean>(backend() SYMBOL rhs.as<multiprecision::integer>());\
    }                                                                          \
    else                                                                       \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << "no viable " OPERATION " with " << *this << " and " << rhs;        \
      throw std::logic_error { ss.str() };                                     \
    }                                                                          \
  } static_assert(true, "semicolon required after this macro")

  DEFINE_COMPARISON(real, ==, "equality comparison");
  DEFINE_COMPARISON(real, !=, "inequality comparison");
  DEFINE_COMPARISON(real, <,  "less-than comparison");
  DEFINE_COMPARISON(real, <=, "less-equal comparison");
  DEFINE_COMPARISON(real, >,  "greater-than comparison");
  DEFINE_COMPARISON(real, >=, "greater-equal comparison");

  DEFINE_COMPARISON(integer, ==, "equality comparison");
  DEFINE_COMPARISON(integer, !=, "inequality comparison");
  DEFINE_COMPARISON(integer, <,  "less-than comparison");
  DEFINE_COMPARISON(integer, <=, "less-equal comparison");
  DEFINE_COMPARISON(integer, >,  "greater-than comparison");
  DEFINE_COMPARISON(integer, >=, "greater-equal comparison");
}} // namespace meevax::kernel

#undef DEFINE_BINARY_ARITHMETIC_INTEGER
#undef DEFINE_BINARY_ARITHMETIC_REAL
#undef DEFINE_COMPARISON
#endif // INCLUDED_MEEVAX_KERNEL_NUMERICAL_HPP
