#ifndef INCLUDED_MEEVAX_UTILITY_DELAY_HPP
#define INCLUDED_MEEVAX_UTILITY_DELAY_HPP

#include <meevax/kernel/forward.hpp>
#include <meevax/utility/demangle.hpp>

namespace meevax
{
inline namespace utility
{
  template <typename F>
  struct delay
  {
    static inline F f {};

    template <typename T, typename U, typename = void>
    struct viable
      : public std::false_type
    {};

    template <typename T, typename U>
    struct viable<T, U, std::void_t<decltype(std::invoke(std::declval<F>(), std::declval<T>(), std::declval<U>()))>>
      : public std::true_type
    {};

    template <typename T, typename U, typename = void>
    struct select
    {
      template <typename R>
      static auto apply(T&&, U&&) -> R
      {
        if constexpr (std::is_same<R, bool>::value)
        {
          return false;
        }
        else
        {
          // TODO USE demangle
          throw make_error("no viable operation ", typeid(F).name(), " with ", typeid(T).name(), " and ", typeid(U).name());
        }
      }
    };

    template <typename T, typename U>
    struct select<T, U, typename std::enable_if<viable<T, U>::value>::type>
    {
      template <typename R>
      static constexpr auto apply(T&& x, U&& y) -> R
      {
        return f(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
      }
    };

    template <typename R, typename... Ts>
    static constexpr auto yield(Ts&&... xs) -> decltype(auto)
    {
      return select<Ts...>().template apply<R>(std::forward<decltype(xs)>(xs)...);
    }
  };

  /* ---- Miscellaneous --------------------------------------------------------
   *
   *  Temporary
   *
   * ------------------------------------------------------------------------ */

  struct read
  {
    template <typename Port, typename... Ts>
    constexpr auto operator ()(Port&& port, Ts&&... xs) const -> decltype(auto)
    {
      return (port >> ... >> xs);
    }
  };

  struct write
  {
    template <typename Port, typename... Ts>
    constexpr auto operator ()(Port&& port, Ts&&... xs) const -> decltype(auto)
    {
      return (port << ... << xs);
    }

    template <typename Port>
    decltype(auto) operator ()(Port&& port, std::string const& datum) const
    {
      return port << "#" << std::quoted(datum);
    }
  };
} // namespace utility
} // namespace meevax

#endif // INCLUDED_MEEVAX_UTILITY_DELAY_HPP
