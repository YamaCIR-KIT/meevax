#ifndef INCLUDED_MEEVAX_SYSTEM_SYMBOL_HPP
#define INCLUDED_MEEVAX_SYSTEM_SYMBOL_HPP

#include <meevax/system/object.hpp>

namespace meevax::system
{
  struct symbol
    : public std::string
  {
    template <typename... Ts>
    explicit constexpr symbol(Ts&&... args)
      : std::string {std::forward<Ts>(args)...}
    {}

    operator std::string() const
    {
      return *this;
    }
  };

  auto operator<<(std::ostream& os, const symbol& symbol)
    -> decltype(os)
  {
    if (symbol.empty())
    {
      return os << highlight::syntax << "#("
                << highlight::constructor << "symbol"
                << attribute::normal << highlight::comment << " #;" << &symbol << attribute::normal
                << highlight::syntax << ")"
                << attribute::normal;
    }
    else
    {
      return os << attribute::normal << static_cast<const std::string&>(symbol);
    }
  }
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_SYMBOL_HPP

