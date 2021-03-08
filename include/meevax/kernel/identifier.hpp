#ifndef INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP
#define INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP

#include <meevax/kernel/syntactic_keyword.hpp>

namespace meevax
{
inline namespace kernel
{
  auto lookup(let const& x, let const& env)
  {
    if (let const& binding = assq(x, env); not binding.eqv(f))
    {
      return cdr(binding);
    }
    else
    {
      return unwrap_syntax(x);
    }
  }
} // namespace kernel
} // namespace meevax

#endif // INCLUDED_MEEVAX_KERNEL_IDENTIFIER_HPP
