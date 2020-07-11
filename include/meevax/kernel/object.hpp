#ifndef INCLUDED_MEEVAX_KERNEL_OBJECT_HPP
#define INCLUDED_MEEVAX_KERNEL_OBJECT_HPP

#include <meevax/kernel/pointer.hpp>

namespace meevax::kernel
{
  /* ==== Identity =============================================================
   *
   * ======================================================================== */
  template <typename T>
  struct alignas(category_mask + 1) identity
  {
    virtual auto type() const noexcept -> const std::type_info&
    {
      return typeid(T);
    }

    #if __cpp_if_constexpr

    virtual auto copy() const -> pointer<T>
    {
      if constexpr (std::is_copy_constructible<T>::value)
      {
        return std::make_shared<T>(static_cast<const T&>(*this));
      }
      else
      {
        std::stringstream port {};
        port << typeid(T).name() << " is not copy_constructible";
        throw std::logic_error { port.str() };
      }
    }

    #else // __cpp_if_constexpr

    template <typename U, typename = void>
    struct if_copy_constructible
    {
      template <typename... Ts>
      static auto call_it(Ts&&...) -> pointer
      {
        std::stringstream port {};
        port << typeid(U).name() << " is not copy_constructible";
        throw std::logic_error { port.str() };
      }
    };

    template <typename U>
    struct if_copy_constructible<U, typename std::enable_if<std::is_copy_constructible<U>::value>::type>
    {
      template <typename... Ts>
      static auto call_it(Ts&&... xs) -> pointer
      {
        return std::make_shared<U>(std::forward<decltype(xs)>(xs)...);
      }
    };

    virtual auto copy() const -> pointer<T>
    {
      return if_copy_constructible<T>::call_it(*this);
    }

    #endif // __cpp_if_constexpr

    virtual bool compare(const pointer<T>& rhs) const
    {
      if constexpr (concepts::equality_comparable<T>::value)
      {
        if (const auto x { std::dynamic_pointer_cast<const T>(rhs) })
        {
          return static_cast<const T&>(*this) == *x;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }

    virtual auto write(std::ostream& os) const -> decltype(os)
    {
      return os << static_cast<const T&>(*this);
    };

    // override by binder's operators
    #define boilerplate(SYMBOL)                                                \
    virtual auto operator SYMBOL(const pointer<T>&) const -> pointer<T>        \
    {                                                                          \
      std::stringstream ss {};                                                 \
      ss << __FILE__ << ":" << __LINE__;                                       \
      throw std::logic_error { ss.str() };                                     \
    } static_assert(true, "semicolon required after this macro")

    boilerplate(*);
    boilerplate(+);
    boilerplate(-);
    boilerplate(/);

    boilerplate(==);
    boilerplate(!=);

    boilerplate(<);
    boilerplate(<=);
    boilerplate(>);
    boilerplate(>=);

    #undef boilerplate
  };

  struct pair; // forward declaration

  using object = pointer<pair>;

  // TODO Rename to 'cons'?
  using resource = std::allocator<object>;

  template <typename T, typename... Ts>
  inline constexpr decltype(auto) make(Ts&&... xs)
  {
    return object::make_binding<T>(std::forward<decltype(xs)>(xs)...);
  }

  template <typename T,
            typename MemoryResource, // XXX (GCC-9 <=)
            typename... Ts>
  inline constexpr decltype(auto) allocate(MemoryResource&& resource, Ts&&... xs)
  {
    return
      object::allocate_binding<T>(
        std::forward<decltype(resource)>(resource),
        std::forward<decltype(xs)>(xs)...);
  }

  static const object unit {nullptr};

  #define DEFINE_GHOST(TYPENAME)                                               \
  struct TYPENAME##_t                                                          \
  {                                                                            \
    friend auto operator<<(std::ostream& os, const TYPENAME##_t&)              \
      -> decltype(os)                                                          \
    {                                                                          \
      return os << console::faint << "#;" #TYPENAME                            \
                << console::reset;                                             \
    }                                                                          \
  };                                                                           \
                                                                               \
  static const auto TYPENAME { make<TYPENAME##_t>() }

  DEFINE_GHOST(undefined);
  DEFINE_GHOST(unspecified);
} // namespace meevax::kernel

#undef DEFINE_BINARY_OPERATOR_ELEVATOR
#endif // INCLUDED_MEEVAX_KERNEL_OBJECT_HPP
