#ifndef INCLUDED_MEEVAX_KERNEL_POINTER_HPP
#define INCLUDED_MEEVAX_KERNEL_POINTER_HPP

#include <atomic>
#include <cassert>
#include <memory>

#include <meevax/functional/compose.hpp>
#include <meevax/memory/tagged_pointer.hpp>
#include <meevax/type_traits/is_equality_comparable.hpp>
#include <meevax/utility/delay.hpp>
#include <meevax/utility/demangle.hpp>
#include <meevax/utility/module.hpp>
#include <meevax/utility/perfect_forward.hpp>

namespace meevax
{
inline namespace kernel
{
  /* ---- Heterogenous Shared Pointer ------------------------------------------
   *
   *  This type requires to the template parameter T inherits top type.
   *
   * ------------------------------------------------------------------------ */
  template <typename T>
  class pointer
    : public std::shared_ptr<T>
  {
    /* ---- Binder -------------------------------------------------------------
     *
     *  The object binder is the actual data pointed to by the pointer type. To
     *  handle all types uniformly, the binder inherits type T and uses dynamic
     *  polymorphism. This provides access to the bound type ID and its
     *  instances. However, the performance is inferior due to the heavy use of
     *  dynamic cast as a price for convenience.
     *
     * ---------------------------------------------------------------------- */
    template <typename B>
    struct binder
      : public virtual T
      , public B
    {
      template <typename... Ts>
      explicit constexpr binder(Ts&&... xs)
        : std::conditional<
            std::is_base_of<T, B>::value, T, B
          >::type { std::forward<decltype(xs)>(xs)... }
      {}

      virtual ~binder() = default;

      std::type_info const& type() const noexcept override
      {
        return typeid(B);
      }

      auto copy() const -> pointer override
      {
        return delay<clone>().yield<pointer>(*this, nullptr);
      }

      auto eqv(pointer const& rhs) const -> bool override
      {
        if constexpr (is_equality_comparable<B>::value)
        {
          auto const p = std::dynamic_pointer_cast<B const>(rhs);
          return p and static_cast<B const&>(*this) == *p;
        }
        else
        {
          return false;
        }
      }

      auto write_to(std::ostream & port) const -> std::ostream & override
      {
        return delay<write>().yield<decltype(port)>(port, static_cast<B const&>(*this));
      }

      /* ---- Numerical operations ------------------------------------------ */

      #define BOILERPLATE(SYMBOL, RESULT, OPERATION)                           \
      auto operator SYMBOL(pointer const& rhs) const -> RESULT override        \
      {                                                                        \
        return delay<OPERATION>().yield<RESULT>(static_cast<B const&>(*this), rhs); \
      } static_assert(true)

      BOILERPLATE(+, pointer, std::plus<void>);
      BOILERPLATE(-, pointer, std::minus<void>);
      BOILERPLATE(*, pointer, std::multiplies<void>);
      BOILERPLATE(/, pointer, std::divides<void>);
      BOILERPLATE(%, pointer, std::modulus<void>);

      BOILERPLATE(==, bool, std::equal_to<void>);
      BOILERPLATE(!=, bool, std::not_equal_to<void>);
      BOILERPLATE(<,  bool, std::less<void>);
      BOILERPLATE(<=, bool, std::less_equal<void>);
      BOILERPLATE(>,  bool, std::greater<void>);
      BOILERPLATE(>=, bool, std::greater_equal<void>);

      #undef BOILERPLATE
    };

  public:
    pointer(const std::shared_ptr<T>& other)
      : std::shared_ptr<T> { other }
    {}

    template <typename B>
    pointer(const std::shared_ptr<binder<B>>& binding)
      : std::shared_ptr<T> { binding }
    {}

    template <typename... Ts>
    explicit constexpr pointer(Ts&&... xs)
      : std::shared_ptr<T> { std::forward<decltype(xs)>(xs)... }
    {}

    template <typename B, typename... Ts, REQUIRES(std::is_compound<B>)>
    static auto bind(Ts&&... xs) -> pointer
    {
      if constexpr (std::is_same<B, T>::value)
      {
        return std::make_shared<T>(std::forward<decltype(xs)>(xs)...);
      }
      else
      {
        return std::make_shared<binder<B>>(std::forward<decltype(xs)>(xs)...);
      }
    }

    auto binding() const noexcept -> decltype(auto)
    {
      return std::shared_ptr<T>::operator *();
    }

    /* ---- Type Predicates ------------------------------------------------- */

    auto type() const -> decltype(auto)
    {
      return *this ?  std::shared_ptr<T>::get()->type() : typeid(null);
    }

    template <typename U>
    auto is() const
    {
      return type() == typeid(typename std::decay<U>::type);
    }

    template <typename U,
              typename std::enable_if<
                std::is_null_pointer<
                  typename std::decay<U>::type
                >::value
              >::type = 0>
    auto is() const
    {
      return not std::shared_ptr<T>::operator bool();
    }

    template <typename U>
    auto is_polymorphically() const
    {
      return std::dynamic_pointer_cast<U>(*this).operator bool();
    }

    /* ---- Accessors ------------------------------------------------------- */

    template <typename U>
    auto as() const -> typename std::add_lvalue_reference<U>::type
    {
      if (auto bound = std::dynamic_pointer_cast<U>(*this); bound)
      {
        return *bound;
      }
      else
      {
        throw make_error("no viable conversion from ", demangle(binding().type()), " to ", demangle(typeid(U)));
      }
    }

    template <typename U,
              typename std::enable_if<is_immediate<U>::value>::type = 0>
    decltype(auto) as() const
    {
      return reinterpret_cast<U>(0); // TODO unbox(std::shared_ptr<T>::get());
    }

    decltype(auto) copy() const
    {
      return binding().copy();
    }

    bool eqv(pointer const& rhs) const
    {
      return type() == rhs.type() and binding().eqv(rhs);
    }

    template <typename... Ts>
    decltype(auto) load(Ts&&...)
    {
      return std::atomic_load(this);
    }

    template <typename... Ts>
    decltype(auto) store(Ts&&... xs)
    {
      return std::atomic_store(this, std::forward<decltype(xs)>(xs)...);
    }

    template <typename... Ts>
    decltype(auto) exchange(Ts&&... xs)
    {
      return std::atomic_exchange(this, std::forward<decltype(xs)>(xs)...);
    }
  };

  template <typename T>
  auto operator <<(output_port & port, pointer<T> const& datum) -> output_port &
  {
    return (datum.template is<null>() ? port << magenta << "()" : datum.binding().write_to(port)) << reset;
  }

  #define BOILERPLATE(SYMBOL)                                                  \
  template <typename T, typename U>                                            \
  decltype(auto) operator SYMBOL(pointer<T> const& a, pointer<U> const& b)     \
  {                                                                            \
    if (a && b)                                                                \
    {                                                                          \
      return a.binding() SYMBOL b;                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      throw make_error("no viable operation '" #SYMBOL " with ", a, " and ", b); \
    }                                                                          \
  } static_assert(true)

  BOILERPLATE(* );
  BOILERPLATE(+ );
  BOILERPLATE(- );
  BOILERPLATE(/ );
  BOILERPLATE(% );

  BOILERPLATE(< );
  BOILERPLATE(<=);
  BOILERPLATE(> );
  BOILERPLATE(>=);

  #undef BOILERPLATE
} // namespace kernel
} // namespace meevax

namespace std
{
  template <typename T>
  class hash<meevax::kernel::pointer<T>>
    : public hash<std::shared_ptr<T>>
  {};
}

#endif // INCLUDED_MEEVAX_KERNEL_POINTER_HPP
