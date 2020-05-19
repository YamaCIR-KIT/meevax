#ifndef INCLUDED_MEEVAX_UTILITY_IMPORT_HPP
#define INCLUDED_MEEVAX_UTILITY_IMPORT_HPP

#include <utility>

// Curiously Recurring Template Pattern (CRTP)

#define IMPORT(FROM, SYMBOL)                                                   \
template <typename... Ts>                                                      \
constexpr decltype(auto) SYMBOL(Ts&&... xs)                                    \
{                                                                              \
  return                                                                       \
    static_cast<FROM&>(*this).FROM::SYMBOL(                                    \
      std::forward<decltype(xs)>(xs)...);                                      \
}

#define IMPORT_CONST(FROM, SYMBOL)                                             \
template <typename... Ts>                                                      \
constexpr decltype(auto) SYMBOL(Ts&&... xs) const                              \
{                                                                              \
  return                                                                       \
    static_cast<const FROM&>(*this).FROM::SYMBOL(                              \
      std::forward<decltype(xs)>(xs)...);                                      \
}

#endif // INCLUDED_MEEVAX_UTILITY_IMPORT_HPP
