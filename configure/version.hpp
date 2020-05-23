#ifndef INCLUDED_MEEVAX_KERNEL_VERSION_HPP
#define INCLUDED_MEEVAX_KERNEL_VERSION_HPP

#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>

#include <meevax/kernel/list.hpp>
#include <meevax/kernel/numerical.hpp>
#include <meevax/kernel/symbol.hpp>
#include <meevax/utility/construct_on_first_use.hpp>

#ifdef major // NOTE: Maybe defined by <sys/sysmacros.h> when _GNU_SOURCE.
#undef major
#endif

#ifdef minor // NOTE: Maybe defined by <sys/sysmacros.h> when _GNU_SOURCE.
#undef minor
#endif

namespace meevax::kernel
{
  constexpr auto boost_version = []() -> std::string
  {
    static constexpr std::size_t major { BOOST_VERSION / 100000 };
    static constexpr std::size_t minor { BOOST_VERSION / 100 % 1000 };
    static constexpr std::size_t patch { BOOST_VERSION % 100 };

    static const auto semantic {
      boost::lexical_cast<std::string>(major) + "." +
      boost::lexical_cast<std::string>(minor) + "." +
      boost::lexical_cast<std::string>(patch)
    };

    return semantic;
  };

  struct version
    : public object
  {
    Construct_On_First_Use(major, make<real>("${PROJECT_VERSION_MAJOR}"));
    Construct_On_First_Use(minor, make<real>("${PROJECT_VERSION_MINOR}"));
    Construct_On_First_Use(patch, make<real>("${PROJECT_VERSION_PATCH}"));

    Construct_On_First_Use(semantic, make<symbol>("${PROJECT_VERSION}"));

    Construct_On_First_Use(libraries,
      static_cast<object>(
        list(
          cons(
            make<symbol>("boost"),
            make<symbol>(boost_version())),
          cons(
            make<symbol>("gmp"),
            make<symbol>(gmp_version)),
          cons(
            make<symbol>("mpfr"),
            make<symbol>(mpfr_get_version()))
          )));

    explicit version()
      : object
        {
          list(
            major(),
            minor(),
            patch())
        }
    {}
  };
} // namespace meevax::kernal

#endif // INCLUDED_MEEVAX_CONFIGURE_VERSION_HPP
