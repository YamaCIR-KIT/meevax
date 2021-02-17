#ifndef INCLUDED_MEEVAX_KERNEL_LIBRARY_HPP
#define INCLUDED_MEEVAX_KERNEL_LIBRARY_HPP

#include <string_view>

namespace meevax
{
inline namespace kernel
{
  #if __cpp_lib_string_view
  using string_view = std::string_view;
  #else
  using string_view = std::experimental::string_view;
  #endif

  extern string_view const define_syntax;
  extern string_view const derived_expression_types;
  extern string_view const dynamic_wind;
  extern string_view const overture;
  extern string_view const srfi_1;
  extern string_view const srfi_8;
  extern string_view const values;
} // namespace kernel
} // namespace meevax

#endif // INCLUDED_MEEVAX_KERNEL_LIBRARY_HPP
