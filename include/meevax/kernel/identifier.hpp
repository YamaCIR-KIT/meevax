#ifndef INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP
#define INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP

#include <meevax/kernel/pair.hpp>

namespace meevax::kernel
{
  /* ==== Identifier ===========================================================
   *
   * Identifier is pair of symbol and syntactic-continuation
   *
   * ======================================================================== */
  struct identifier
    : public virtual pair
  {
    using pair::pair;

    friend auto operator <<(std::ostream& os, const identifier& i) -> decltype(os)
    {
      return os << console::magenta << "#,("
                << console::green << "syntax " // See R4RS p.44
                << console::reset << i.first
                << console::magenta << ")"
                << console::reset;
    }
  };
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP
