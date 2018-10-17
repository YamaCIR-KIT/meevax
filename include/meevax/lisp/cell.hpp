#ifndef INCLUDED_MEEVAX_LISP_CELL_HPP
#define INCLUDED_MEEVAX_LISP_CELL_HPP

#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <boost/cstdlib.hpp>

#include <meevax/utility/binder.hpp>
#include <meevax/utility/recursive_binary_tuple_iterator.hpp>

namespace meevax::lisp
{
  class cell;

  using cursor = utility::recursive_binary_tuple_iterator<cell>;

  template <typename T, typename... Ts>
  [[deprecated]] cursor make_as(Ts&&... args)
  {
    using binder = meevax::utility::binder<T, cell>;
    return std::make_shared<binder>(std::forward<Ts>(args)...);
  }
} // namespace meevax::lisp

#include <meevax/lisp/table.hpp>

namespace meevax::lisp
{
  struct symbol_generator
  {
    using binder = utility::binder<std::string, cell>;

    template <typename... Ts>
    cursor operator()(Ts&&... args)
    {
      return std::make_shared<binder>(std::forward<decltype(args)>(args)...);
    }
  };

  heterogeneous_dictionary<cursor, symbol_generator> symbols {
    std::make_pair("nil", cursor {nullptr})
  };

  struct cell
    : public std::pair<cursor, cursor>
  {
    template <typename T>
    constexpr cell(T&& head)
      : std::pair<cursor, cursor> {std::forward<T>(head), symbols.intern("nil")}
    {}

    template <typename... Ts>
    constexpr cell(Ts&&... args)
      : std::pair<cursor, cursor> {std::forward<Ts>(args)...}
    {}

    template <typename T>
    auto as() const
    {
      return dynamic_cast<const T&>(*this);
    }

    virtual auto type() const noexcept
      -> const std::type_info&
    {
      return typeid(cell);
    }
  };

  template <typename T>
  bool atom(T&& e)
  {
    static const std::unordered_map<std::type_index, bool> dispatch
    {
      {typeid(cell), false},
      {typeid(std::string), true}
    };

    return !e || dispatch.at(e.access().type());
  }

  auto cons = [](auto&&... args) -> cursor
  {
    return std::make_shared<cell>(std::forward<decltype(args)>(args)...);
  };

  template <typename T, typename U>
  decltype(auto) operator|(T&& head, U&& tail)
  {
    return cons(std::forward<T>(head), std::forward<U>(tail));
  }
} // namespace meevax::lisp

#endif // INCLUDED_MEEVAX_LISP_CELL_HPP

