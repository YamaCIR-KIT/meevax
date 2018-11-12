#ifndef INCLUDED_MEEVAX_LISP_CELL_HPP
#define INCLUDED_MEEVAX_LISP_CELL_HPP

#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <meevax/utility/type_erasure.hpp>
#include <meevax/utility/heterogeneous_dictionary.hpp>
#include <meevax/utility/recursive_binary_tuple_iterator.hpp>

namespace meevax::lisp
{
  class cell;

  using cursor = utility::recursive_binary_tuple_iterator<cell>;

  template <typename T>
  struct bind
  {
    template <typename... Ts>
    cursor operator()(Ts&&... args)
    {
      using binder = utility::binder<T, cell>;
      return std::make_shared<binder>(std::forward<Ts>(args)...);
    }
  };

  utility::heterogeneous_dictionary<cursor, bind<std::string>> symbols {
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

    virtual ~cell() = default;

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
} // namespace meevax::lisp

#endif // INCLUDED_MEEVAX_LISP_CELL_HPP

