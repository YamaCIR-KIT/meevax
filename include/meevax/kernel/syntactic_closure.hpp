#ifndef INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CLOSURE_HPP
#define INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CLOSURE_HPP

#include <meevax/kernel/list.hpp>
#include <meevax/kernel/symbol.hpp>

namespace meevax
{
inline namespace kernel
{
  struct syntactic_closure
    : public virtual pair
  {
    using pair::pair;

    auto form() const noexcept -> decltype(auto) { return car(*this); }
    auto form()       noexcept -> decltype(auto) { return car(*this); }

    auto syntactic_environment() const noexcept -> decltype(auto) { return cdr(*this); }
    auto syntactic_environment()       noexcept -> decltype(auto) { return cdr(*this); }

    auto lookup() const
    {
      return assq(form(), syntactic_environment());
    }

    auto strip() const
    {
      const auto pare { lookup() };
      return pare.eqv(f) ? form() : cadr(pare); // TODO cadr => cdr
    }

    auto is_identifier() const
    {
      return form().is<symbol>();
    }

    auto is_free() const
    {
      return lookup().eqv(f);
    }

    auto is_bound() const
    {
      return not is_free();
    }

    friend auto operator <<(std::ostream& port, const syntactic_closure& datum) -> decltype(auto)
    {
      return port << underline << datum.form() << faint << " #;" << &datum << reset;
    }
  };
} // namespace kernel
} // namespace meevax

#endif // INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CLOSURE_HPP
