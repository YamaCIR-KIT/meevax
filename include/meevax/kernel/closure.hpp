#ifndef INCLUDED_MEEVAX_KERNEL_CLOSURE_HPP
#define INCLUDED_MEEVAX_KERNEL_CLOSURE_HPP

#include <meevax/kernel/pair.hpp>

namespace meevax::kernel
{
  // closure is pair of expression and lexical-environment
  struct closure
    : public virtual pair
  {
    using identity = closure;

    using pair::pair; // inheriting constructors

    friend auto operator <<(std::ostream& os, const identity& i)
      -> decltype(os)
    {
      return os << highlight::syntax << "#("
                << highlight::type << "closure"
                << attribute::normal << highlight::comment << " #;" << &i << attribute::normal
                << highlight::syntax << ")"
                << attribute::normal;
    }
  };
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_CLOSURE_HPP

