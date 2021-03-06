#ifndef INCLUDED_MEEVAX_KERNEL_SYNTAX_HPP
#define INCLUDED_MEEVAX_KERNEL_SYNTAX_HPP

#include <meevax/kernel/object.hpp>
#include <meevax/kernel/syntactic_context.hpp>

#define SYNTAX(NAME)                                                           \
  auto NAME(                                                                   \
    [[maybe_unused]] syntactic_context const& the_expression_is,               \
    [[maybe_unused]] syntactic_continuation & current_syntactic_continuation,  \
    [[maybe_unused]] let const& expression,                                    \
    [[maybe_unused]] let const& frames,                                        \
    [[maybe_unused]] let const& continuation) -> let const

namespace meevax
{
inline namespace kernel
{
  class syntactic_continuation;

  struct syntax
    : public std::function<SYNTAX()>
  {
    using signature = SYNTAX((*));

    std::string const name;

    template <typename... Ts>
    explicit syntax(std::string const& name, Ts&&... xs)
      : std::function<SYNTAX()> { std::forward<decltype(xs)>(xs)...  }
      , name { name }
    {}

    template <typename... Ts>
    decltype(auto) compile(Ts&&... xs)
    {
      return (*this)(std::forward<decltype(xs)>(xs)...);
    }
  };

  auto operator <<(output_port &, syntax const&) -> output_port &;
} // namespace kernel
} // namespace meevax

#endif // INCLUDED_MEEVAX_KERNEL_SYNTAX_HPP
