#ifndef INCLUDED_MEEVAX_KERNEL_PROCEDURE_HPP
#define INCLUDED_MEEVAX_KERNEL_PROCEDURE_HPP

#include <functional> // std::funstion
#include <numeric> // std::accumulate

#include <meevax/kernel/list.hpp>

#define PROCEDURE(NAME)                                                        \
  const meevax::kernel::object NAME(                                           \
    [[maybe_unused]] const meevax::kernel::resource& resource,                 \
    [[maybe_unused]] const meevax::kernel::homoiconic_iterator& operands)

namespace meevax::kernel
{
  struct procedure
    : public std::function<PROCEDURE()>
  {
    using signature = PROCEDURE((*));

    const std::string name;

    template <typename... Ts>
    explicit procedure(const std::string& name, Ts&&... operands)
      : std::function<PROCEDURE()> {
          std::forward<decltype(operands)>(operands)...
        }
      , name {name}
    {}
  };

  std::ostream& operator<<(std::ostream& os, const procedure& procedure)
  {
    return os << highlight::syntax << "#("
              << highlight::constructor << "procedure"
              << attribute::normal << " " << procedure.name
              << highlight::comment << " #;" << &procedure << attribute::normal
              << highlight::syntax << ")"
              << attribute::normal;
  }
} // namespace meevax::kernel

#define MEEVAX_API_BOOLEAN(...)                                                \
  (__VA_ARGS__ ? meevax::kernel::true_object : meevax::kernel::false_object)

#define MEEVAX_TYPE_PREDICATE(...)                                             \
  MEEVAX_API_BOOLEAN(                                                          \
    meevax::kernel::car(operands).is<__VA_ARGS__>())

// TODO Rename simply "MEEVAX_FOLD"
#define MEEVAX_FOLD_ARGUMENTS(INIT, ...) \
std::accumulate(std::begin(operands), std::end(operands), INIT, __VA_ARGS__)

#define MEEVAX_BINARY_OPERATION(...) \
std::invoke(__VA_ARGS__, meevax::kernel::car(operands), meevax::kernel::cadr(operands))

#endif // INCLUDED_MEEVAX_KERNEL_PROCEDURE_HPP

