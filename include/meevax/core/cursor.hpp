#ifndef INCLUDED_MEEVAX_CORE_CURSOR_HPP
#define INCLUDED_MEEVAX_CORE_CURSOR_HPP

#include <functional> // std::hash
#include <iterator>

#include <meevax/core/accessor.hpp>
#include <meevax/core/pair.hpp>

namespace meevax::core
{
  struct cursor
    : public accessor<pair>,
      public std::iterator<std::input_iterator_tag, cursor>
  {
    template <typename... Ts>
    constexpr cursor(Ts&&... args)
      : accessor<pair> {std::forward<Ts>(args)...}
    {}

    decltype(auto) operator*() const
    {
      return std::data(*this).first;
    }

    decltype(auto) operator->() const
    {
      return operator*();
    }

    decltype(auto) operator++()
    {
      return *this = std::data(*this).second;
    }
  };

  const cursor unit {nullptr};

  cursor begin(const accessor<pair>& pair) noexcept
  {
    return pair;
  }

  cursor end(const accessor<pair>& pair) noexcept
  {
    return unit;
  }
} // namespace meevax::core

namespace std
{
  template <typename T>
  struct hash;

  template <>
  struct hash<meevax::core::cursor>
    : public std::hash<std::shared_ptr<meevax::core::pair>>
  {};
} // namespace std

#endif // INCLUDED_MEEVAX_CORE_CURSOR_HPP

