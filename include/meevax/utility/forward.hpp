#ifndef INCLUDED_MEEVAX_UTILITY_FORWARD_HPP
#define INCLUDED_MEEVAX_UTILITY_FORWARD_HPP

// Reference: https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html

#include <utility>

namespace meevax
{
inline namespace utility
{
  #define FORWARD(...) \
    std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

  template <typename... Ts>
  constexpr decltype(auto) forward_capture(Ts&&... xs)
  {
    return std::tuple<Ts...>(FORWARD(xs)...);
  }

  #define FORWARD_CAPTURE(...) \
    forward_capture(FORWARD(__VA_ARGS__))

  template <typename... Ts>
  constexpr decltype(auto) forward_captures(Ts&&... xs)
  {
    return std::make_tuple(FORWARD_CAPTURE(xs)...);
  }

  #define FORWARD_VARIADIC_CAPTURE(...) \
    forward_captures(FORWARD(__VA_ARGS__)...)

  template <typename T>
  constexpr decltype(auto) captured(T&& x)
  {
    return std::get<0>(FORWARD(x));
  }
} // namespace utility
} // namespace meevax

#endif // INCLUDED_MEEVAX_UTILITY_FORWARD_HPP
