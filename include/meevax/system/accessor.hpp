#ifndef INCLUDED_MEEVAX_SYSTEM_ACCESSOR_HPP
#define INCLUDED_MEEVAX_SYSTEM_ACCESSOR_HPP

#include <iostream> // std::ostream
#include <memory> // std::shared_ptr
#include <type_traits> // std::conditional
#include <typeinfo> // typeid
#include <utility> // std::forward

namespace meevax::system
{
  template <typename T>
  struct facade
  {
    virtual auto type() const noexcept
      -> const std::type_info&
    {
      return typeid(T);
    }

    // Type T is able to customize print function via stream output operator.
    virtual std::ostream& write(std::ostream& os) const
    {
      return os << static_cast<const T&>(*this);
    };
  };

  template <typename TopType = facade<void>>
  struct accessor
    : public std::shared_ptr<TopType>
  {
    // We use parallel inheritance to bind any typed object because there is no
    // guarantiee for bound type is static-castable from/to universal base type.
    template <typename BoundType>
    struct binder
      : public BoundType
      , public virtual TopType
    {
      template <typename... Ts>
      explicit constexpr binder(Ts&&... args)
        : std::conditional< // XXX
            std::is_base_of<TopType, BoundType>::value, TopType, BoundType
          >::type {std::forward<Ts>(args)...}
      {}

      auto type() const noexcept
        -> const std::type_info& override
      {
        return typeid(BoundType);
      }

    private:
      // Override TopType::write(), then invoke BoundType's stream output operator.
      std::ostream& write(std::ostream& os) const override
      {
        return os << static_cast<const BoundType&>(*this);
      }
    };

  public:
    template <typename... Ts>
    constexpr accessor(Ts&&... args)
      : std::shared_ptr<TopType> {std::forward<Ts>(args)...}
    {}

    // If you initialize accessor<TopType> by accessor<TopType>::bind<BoundType>(args...),
    // std::shared_ptr<TopType> remembers it has assigned accessor<TopType>::binder<BoundType> originally,
    // thus both TopType and BoundType's destructor will works correctly.
    template <typename BoundType, typename... Ts>
    static constexpr auto bind(Ts&&... args)
      -> accessor<TopType>
    {
      using bindings = binder<BoundType>;
      return std::make_shared<bindings>(std::forward<Ts>(args)...);
    }

    decltype(auto) access()       { return std::shared_ptr<TopType>::operator*(); }
    decltype(auto) access() const { return std::shared_ptr<TopType>::operator*(); }

    template <typename T>
    decltype(auto) is() const
    {
      return access().type() == typeid(T);
    }

    template <typename T>
    decltype(auto) as()
    {
      return dynamic_cast<T&>(access());
    }

    template <typename T>
    decltype(auto) as() const
    {
      return dynamic_cast<T&>(access());
    }
  };

  // Invoke TopType::write()
  template <typename T>
  std::ostream& operator<<(std::ostream& os, const accessor<T>& rhs)
  {
    // TODO Provide custamizable printer for nullptr.
    return !rhs ? (os << "()") : rhs.access().write(os);
  }
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_ACCESSOR_HPP

