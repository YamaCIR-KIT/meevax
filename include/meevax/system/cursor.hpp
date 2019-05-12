#ifndef INCLUDED_MEEVAX_SYSTEM_CURSOR_HPP
#define INCLUDED_MEEVAX_SYSTEM_CURSOR_HPP

#include <iterator>

#include <meevax/system/pair.hpp>

namespace meevax::system
{
  // struct cursor;
  // extern "C" const cursor unit;
  // extern "C" const cursor unbound;
  // extern "C" const cursor undefined;

  struct cursor // provides STL supports to cons-cells
    : public objective
    , public std::iterator<std::input_iterator_tag, cursor>
  {
    using reference = cursor&;
    using const_reference = const reference;

    using size_type = std::size_t;

    template <typename... Ts>
    constexpr cursor(Ts&&... args)
      : objective {std::forward<Ts>(args)...}
    {}

  public: // iterator supports
    const auto& operator*() const { return car(*this); }
          auto& operator*()       { return car(*this); }

    decltype(auto) operator->()
    {
      return operator*();
    }

    decltype(auto) operator++()
    {
      return *this = cdr(*this);
    }

  public: // algorithm supports
    const auto& begin() const noexcept { return *this; }
          auto  begin()       noexcept { return *this; }

    const cursor end() const noexcept { return unit; }

  public: // container adapter supports (for std::stack<cursor, cursor>)
    const auto& back() const { return operator*(); }
          auto  back()       { return operator*(); }

    template <typename... Ts>
    decltype(auto) emplace_back(Ts&&... args)
    {
      *this = cons(std::forward<Ts>(args)..., *this);
      return back();
    }

    template <typename... Ts>
    decltype(auto) push_back(Ts&&... args)
    {
      emplace_back(std::forward<Ts>(args)...);
      return back();
    }

    decltype(auto) pop_back() // returns removed element
    {
      const auto buffer {back()};
      operator++();
      return buffer;
    }

  public: // direct stack-like operations
    // 標準スタックよりもVM記述用に特化した気の利いた形式を
    decltype(auto) top() noexcept
    {
      return back();
    }

    decltype(auto) empty() const noexcept
    {
      return *this == unit;
    }

    decltype(auto) size() const
    {
      return std::distance(std::begin(*this), std::end(*this));
    }

    template <typename... Ts>
    decltype(auto) push(Ts&&... args)
    {
      return push_back(std::forward<Ts>(args)...);
    }

    // TODO cursor::push(make<T>(...)) => cursor::emplace<T>(...)
    template <typename... Ts>
    decltype(auto) emplace(Ts&&... args)
    {
      return emplace_back(std::forward<Ts>(args)...);
    }

    template <typename... Ts>
    void pop(Ts&&... args)
    {
      std::advance(*this, std::forward<Ts>(args)...);
    }

    // pop() is NOT special case of pop(1) above.
    decltype(auto) pop() // TODO RENAME
    {
      return pop_back();
    }
  };

  // template <typename T, typename... Ts>
  // constexpr cursor make(Ts&&... args)
  // {
  //   return cursor::bind<T>(std::forward<Ts>(args)...);
  // }
} // namespace meevax::system

namespace std
{
  template <typename>
  struct hash;

  template <>
  struct hash<meevax::system::cursor>
    : public std::hash<std::shared_ptr<meevax::system::pair>>
  {};
} // namespace std

#endif // INCLUDED_MEEVAX_SYSTEM_CURSOR_HPP

