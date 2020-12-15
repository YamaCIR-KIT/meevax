#include <meevax/kernel/number.hpp>
#include <meevax/kernel/ratio.hpp>
#include <meevax/posix/vt102.hpp>

namespace meevax
{
inline namespace kernel
{
  auto ratio::is_integer() const -> bool
  {
    return cdr(*this).as<exact_integer>() == 1;
  }

  auto ratio::reduce() -> ratio const&
  {
    if (const exact_integer divisor {
          boost::multiprecision::gcd(numerator().as<exact_integer>().value, denominator().as<exact_integer>().value)
        };
        divisor != 1)
    {
      car(*this) = make(car(*this).as<exact_integer>() / divisor);
      cdr(*this) = make(cdr(*this).as<exact_integer>() / divisor);
    }

    return *this;
  }

  auto operator <<(output_port & port, ratio const& datum) -> output_port &
  {
    return port << cyan << car(datum)
                << cyan << "/"
                << cyan << cdr(datum) << reset;
  }

  #define BOILERPLATE(SYMBOL)                                                  \
  auto operator SYMBOL(ratio const& lhs, exact_integer const& rhs) -> bool     \
  {                                                                            \
    if (auto copy { lhs }; copy.reduce().is_integer())                         \
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

  auto operator + (ratio const& a, ratio const& b) -> ratio { return ratio(a.numerator() * b.denominator() + b.numerator() * a.denominator(), a.denominator() * b.denominator()); }
  auto operator - (ratio const& a, ratio const& b) -> ratio { return ratio(a.numerator() * b.denominator() - b.numerator() * a.denominator(), a.denominator() * b.denominator()); }
  auto operator * (ratio const& a, ratio const& b) -> ratio { return ratio(a.numerator() *                   b.numerator(),                   a.denominator() * b.denominator()); }
  auto operator / (ratio const& a, ratio const& b) -> ratio { return a * b.invert(); }
  auto operator % (ratio const&,   ratio const& b) -> ratio { return b; } // TODO
  auto operator ==(ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() == (b.numerator() * a.denominator()); }
  auto operator !=(ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() != (b.numerator() * a.denominator()); }
  auto operator < (ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() <  (b.numerator() * a.denominator()); }
  auto operator <=(ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() <= (b.numerator() * a.denominator()); }
  auto operator > (ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() >  (b.numerator() * a.denominator()); }
  auto operator >=(ratio const& a, ratio const& b) -> bool { return (a.numerator() * b.denominator()).binding() >= (b.numerator() * a.denominator()); }
} // namespace kernel
} // namespace meevax
