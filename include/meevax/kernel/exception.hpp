#ifndef INCLUDED_MEEVAX_KERNEL_EXCEPTION_HPP
#define INCLUDED_MEEVAX_KERNEL_EXCEPTION_HPP

#include <sstream>

#include <meevax/kernel/object.hpp>

/* ==== The Exception Tower ===================================================
*
* TODO: Reduce the hierarchy. warning category is unused.
*
* - exception
*    │
*    ├── error                                                       (category)
*    │    │
*    │    ├── configuration_error                                     (section)
*    │    │
*    │    ├── evaluation_error                                        (section)
*    │    │
*    │    ├── reader_error                                            (section)
*    │    │    ├── reader_error_about_pair                              (about)
*    │    │    └── reader_error_about_parentheses                       (about)
*    │    │
*    │    ├── syntax_error                                            (section)
*    │    │    ├── syntax_error_about_assignment                        (about)
*    │    │    └── syntax_error_about_internal_define                   (about)
*    │    │
*    │    └── kernel_error                                            (section)
*    │
*    └── warning                                                     (category)
*
*=========================================================================== */

namespace meevax::kernel
{
  template <typename Ex, typename... Ts>
  void raise(Ts&&... operands)
  {
    Ex {std::forward<decltype(operands)>(operands)...}.raise();
  }

  struct exception
    : public std::runtime_error
  {
    using std::runtime_error::runtime_error;

    template <typename... Ts>
    explicit exception(Ts&&... xs)
      : std::runtime_error {to_string(std::forward<decltype(xs)>(xs)...)}
    {}

    virtual ~exception() = default;

    virtual void raise() const
    {
      throw *this;
    }

  protected:
    template <typename... Ts>
    auto to_string(Ts&&... xs)
    {
      std::stringstream port {};
      (port << ... << xs);
      return port.str();
    }
  };

  #define DEFINE_EXCEPTION_EXTERNAL_REPRESENTATION(TYPENAME, ...)              \
  auto operator<<(std::ostream& os, const TYPENAME& exception)                 \
    -> decltype(auto)                                                          \
  {                                                                            \
    return os << highlight::syntax << "#("                                     \
              << highlight::type << __VA_ARGS__                                \
              << highlight::datum << " " <<  std::quoted(exception.what())     \
              << highlight::syntax << ")"                                      \
              << attribute::normal;                                            \
  }

  DEFINE_EXCEPTION_EXTERNAL_REPRESENTATION(exception, "exception")

  #define DEFINE_EXCEPTION_CATEGORY(CATEGORY)                                  \
  struct [[maybe_unused]] CATEGORY                                             \
    : public exception                                                         \
  {                                                                            \
    template <typename... Ts>                                                  \
    explicit constexpr CATEGORY(Ts&&... operands)                              \
      : exception {std::forward<decltype(operands)>(operands)...}              \
    {}                                                                         \
                                                                               \
    virtual void raise() const override                                        \
    {                                                                          \
      throw *this;                                                             \
    }                                                                          \
  };                                                                           \
                                                                               \
  DEFINE_EXCEPTION_EXTERNAL_REPRESENTATION(CATEGORY, #CATEGORY)

  DEFINE_EXCEPTION_CATEGORY(error)
  DEFINE_EXCEPTION_CATEGORY(warning)

  #define DEFINE_EXCEPTION_SECTION(PREFIX, CATEGORY)                           \
  struct [[maybe_unused]] PREFIX##_##CATEGORY                                  \
    : public CATEGORY                                                          \
  {                                                                            \
    template <typename... Ts>                                                  \
    explicit constexpr PREFIX##_##CATEGORY(Ts&&... operands)                   \
      : CATEGORY {std::forward<decltype(operands)>(operands)...}               \
    {}                                                                         \
                                                                               \
    virtual void raise() const override                                        \
    {                                                                          \
      throw *this;                                                             \
    }                                                                          \
  };                                                                           \
                                                                               \
  DEFINE_EXCEPTION_EXTERNAL_REPRESENTATION(PREFIX##_##CATEGORY, #PREFIX "-" #CATEGORY)

  DEFINE_EXCEPTION_SECTION(configuration, error)
  DEFINE_EXCEPTION_SECTION(evaluation, error)
  DEFINE_EXCEPTION_SECTION(reader, error)
  DEFINE_EXCEPTION_SECTION(syntax, error)
  DEFINE_EXCEPTION_SECTION(kernel, error)

  DEFINE_EXCEPTION_SECTION(configuration, warning)
  DEFINE_EXCEPTION_SECTION(evaluation, warning)
  DEFINE_EXCEPTION_SECTION(reader, warning)
  DEFINE_EXCEPTION_SECTION(syntax, warning)
  DEFINE_EXCEPTION_SECTION(kernel, warning)

  #define DEFINE_EXCEPTION_ABOUT(ABOUT, SECTION, CATEGORY)                     \
  struct [[maybe_unused]] SECTION##_##CATEGORY##_about_##ABOUT                 \
    : public SECTION##_##CATEGORY                                              \
  {                                                                            \
    template <typename... Ts>                                                  \
    explicit constexpr SECTION##_##CATEGORY##_about_##ABOUT(Ts&&... operands)  \
      : SECTION##_##CATEGORY {std::forward<decltype(operands)>(operands)...}   \
    {}                                                                         \
                                                                               \
    virtual void raise() const override                                        \
    {                                                                          \
      throw *this;                                                             \
    }                                                                          \
  };                                                                           \
                                                                               \
  DEFINE_EXCEPTION_EXTERNAL_REPRESENTATION(                                    \
    SECTION##_##CATEGORY##_about_##ABOUT,                                      \
    #SECTION "-" #CATEGORY "-about-" #ABOUT)

  DEFINE_EXCEPTION_ABOUT(character, reader, error)
  DEFINE_EXCEPTION_ABOUT(pair, reader, error)
  DEFINE_EXCEPTION_ABOUT(parentheses, reader, error)

  DEFINE_EXCEPTION_ABOUT(assignment, syntax, error)
  DEFINE_EXCEPTION_ABOUT(internal_define, syntax, error)

  DEFINE_EXCEPTION_ABOUT(pair, kernel, error)
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_EXCEPTION_HPP

