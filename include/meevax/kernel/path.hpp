#ifndef INCLUDED_MEEVAX_KERNEL_PATH_HPP
#define INCLUDED_MEEVAX_KERNEL_PATH_HPP

#include <experimental/filesystem>

#include <meevax/kernel/object.hpp>

namespace meevax { inline namespace kernel
{
  struct path
    : public std::experimental::filesystem::path
  {
    using std::experimental::filesystem::path::path;

    friend auto operator<<(std::ostream& os, const path& p)
      -> decltype(os)
    {
      return os << console::cyan << "#p\"" << p.c_str() << "\""
                << console::reset;
    }
  };
}} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_PATH_HPP
