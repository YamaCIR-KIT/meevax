#include <meevax/kernel/symbol.hpp>
#include <meevax/posix/vt102.hpp>

namespace meevax
{
inline namespace kernel
{
  auto operator <<(std::ostream & port, symbol const& datum) -> decltype(port)
  {
    if (std::empty(datum))
    {
      /* ---- R7RS 2.1. Identifiers --------------------------------------------
       *
       *  Note that || is a valid identifier that is different from any other
       *  identifier.
       *
       * -------------------------------------------------------------------- */
      return port << "||";
    }
    else
    {
      return port << static_cast<bytestring const&>(datum);
    }
  }
} // namespace kernel
} // namespace meevax
