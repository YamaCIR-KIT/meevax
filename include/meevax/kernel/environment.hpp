#ifndef INCLUDED_MEEVAX_KERNEL_ENVIRONMENT_HPP
#define INCLUDED_MEEVAX_KERNEL_ENVIRONMENT_HPP

#include <algorithm> // std::equal
#include <numeric> // std::accumulate

/**
 * Global configuration generated by CMake before compilation.
 */
#include <meevax/kernel/configurator.hpp>
#include <meevax/kernel/machine.hpp>
#include <meevax/kernel/reader.hpp>
#include <meevax/kernel/file.hpp>
#include <meevax/posix/linker.hpp>

/******************************************************************************
* Embedded Source Codes
*
*   library/layer-1.ss
*
******************************************************************************/
extern char _binary_layer_1_ss_start;
extern char _binary_layer_1_ss_end;

namespace meevax::kernel
{
  /****************************************************************************
  * Standard Environment Layers
  *
  *   Layer 0 - Pure Syntax
  *   Layer 1 - Derived Expressions and Standard Procedures
  *
  ****************************************************************************/
  template <int Layer>
  static constexpr std::integral_constant<int, Layer> layer {};

  class environment
    /*
     * The environment is a pair of "current expression" and "global environment
     * (simple association list)". It also has the aspect of a meta-closure that
     * closes the global environment when it constructed (this feature is known
     * as syntactic-closure).
     */
    // : public closure
    : public virtual pair

    /*
     * Reader access symbol table of this environment (by member function
     * "intern") via static polymorphism. The environment indirectly inherits
     * the non-copyable class std::istream (reader base class), so it cannot be
     * copied.
     */
    , public reader<environment>

    /*
     * Each environment has one virtual machine and compiler.
     */
    , public machine<environment>

    /*
     * Global configuration is shared in all of environments running on same
     * process. Thus, any change of configuration member influences any other
     * environments immediately.
     */
    , public configurator<environment>
  {
    std::unordered_map<std::string, object> symbols;

    std::unordered_map<object, object> bindings;

    static inline std::unordered_map<std::string, posix::linker> linkers {};

  public: // Constructors
    // for macro
    environment() = default;

    // for bootstrap scheme-report-environment
    template <int Layer>
    environment(std::integral_constant<int, Layer>);

    // for library constructor
    template <typename... Ts>
    constexpr environment(Ts&&... operands)
      : pair {std::forward<decltype(operands)>(operands)...}
    {}

  public: // Interfaces
    auto ready() const noexcept
    {
      return reader<environment>::ready();
    }

    template <typename T, typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return kernel::machine<environment>::define(
               intern(name),
               make<T>(name, std::forward<decltype(operands)>(operands)...)
             );
    }

    template <typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return kernel::machine<environment>::define(
               intern(name), std::forward<decltype(operands)>(operands)...
             );
    }

    const auto& intern(const std::string& s)
    {
      if (auto iter {symbols.find(s)}; iter != std::end(symbols))
      {
        return iter->second;
      }
      else
      {
        iter = symbols.emplace(s, make<symbol>(s)).first;
        return iter->second;
      }
    }

    const auto& rename(const object& object)
    {
      if (not object.is<symbol>())
      {
        if (verbose == true_object or verbose_environment == true_object)
        {
          std::cerr << "; environment\t; " << object << std::endl;
        }

        return object;
      }
      else
      {
        if (verbose == true_object or verbose_environment == true_object)
        {
          std::cerr << "; environment\t; rename " << object << std::endl;
        }

        return intern(object.as<symbol>());
      }
    }

    decltype(auto) export_(const object& key, const object& value)
    {
      if (auto iter {bindings.find(key)}; iter != std::end(bindings))
      {
        if (verbose == true_object or verbose_environment == true_object)
        {
          std::cerr << "; export\t; detected redefinition (interactive mode ignore previous definition)" << std::endl;
        }

        bindings.at(key) = value;
        return bindings.find(key);
      }
      else
      {
        if (verbose == true_object or verbose_environment == true_object)
        {
          std::cerr << "; export\t; exporting new binding (" << key << " . " << value << ")" << std::endl;
        }

        return bindings.emplace(key, value).first;
      }
    }

    decltype(auto) current_expression()
    {
      return car(std::get<0>(*this));
    }

    decltype(auto) lexical_environment()
    {
      return cdr(std::get<0>(*this));
    }

    decltype(auto) interaction_environment() noexcept
    {
      return static_cast<stack&>(std::get<1>(*this));
    }

    decltype(auto) expand(const object& operands)
    {
      std::cerr << "; macroexpand\t; " << operands << std::endl;

      // std::cerr << "DEBUG! operands = " << operands << std::endl;
      // std::cerr << "DEBUG! lexical = " << lexical_environment() << std::endl;
      // std::cerr << "DEBUG! " << cons(operands, lexical_environment()) << std::endl;

      s = unit;
      e = cons(operands, lexical_environment());
      c = current_expression();
      d = cons(
            unit,         // s
            unit,         // e
            list(_stop_), // c
            unit          // d
          );

      const auto result {execute()};
      // std::cerr << "; \t\t; " << result << std::endl;
      return result;
    }

    template <typename... Ts>
    constexpr decltype(auto) evaluate(Ts&&... operands)
    {
      return execute(compile(std::forward<decltype(operands)>(operands)...));
    }

    const auto& dynamic_link(const std::string& path)
    {
      if (auto iter {linkers.find(path)}; iter != std::end(linkers))
      {
        return iter->second;
      }
      else
      {
        iter = linkers.emplace(path, path).first;
        return iter->second;
      }
    }

    auto locate_library(const object& name)
    {
      for (const object& each : interaction_environment())
      {
        if (const object& key {car(each)}; not key.is<symbol>())
        {
          if (is_same(key, name))
          {
            return cadr(each);
          }
        }
      }

      return unit;
    }

    [[deprecated]] auto import_library(const object& library, const object& continuation)
    {
      stack executable {continuation};

      assert(library.is<environment>());

      for (const object& each : library.as<environment>().interaction_environment())
      {
        executable.push(
          _load_literal_, cadr(each),
          _define_, rename(car(each))
        );
      }

      return executable;
    }

    auto import_(const object& library, const object& continuation)
    {
      auto& source {library.as<environment>()};

      if (source.bindings.empty())
      {
        source.expand(list(library));

        if (source.bindings.empty())
        {
          throw syntax_error {library, " is may not be library"};
        }
      }

      stack executable {continuation};

      for (const auto& [key, value] : source.bindings)
      {
        executable.push(_load_literal_, value, _define_, rename(key));
      }

      return executable;
    }

    template <typename... Ts>
    decltype(auto) load(Ts&&... operands)
    {
      const std::string path {std::forward<decltype(operands)>(operands)...};

      if (verbose == true_object or verbose_loader == true_object)
      {
        std::cerr << "; loader\t; open \"" << path << "\" => ";
      }

      if (std::fstream stream {path}; stream)
      {
        if (verbose == true_object or verbose_loader == true_object)
        {
          std::cerr << "succeeded" << std::endl;
        }

        d.push(s, e, c);
        s = e = c = unit;

        for (auto e {read(stream)}; e != characters.at("end-of-file"); e = read(stream))
        {
          if (verbose == true_object or verbose_reader == true_object)
          {
            std::cerr << "; read\t\t; " << e << std::endl;
          }

          evaluate(e);
        }

        s = d.pop();
        e = d.pop();
        c = d.pop();

        return unspecified;
      }
      else
      {
        if (verbose == true_object or verbose_loader == true_object)
        {
          std::cerr << "failed" << std::endl;
        }

        throw evaluation_error {"failed to open file ", std::quoted(path)};
      }
    }
  };

  template <>
  environment::environment(std::integral_constant<int, 0>)
  {
    define<syntax>("quote", [&](auto&&... operands)
    {
      return quotation(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("if", [&](auto&&... operands)
    {
      return conditional(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("define", [&](auto&&... operands)
    {
      return definition(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("begin", [&](auto&&... operands)
    {
      return sequence(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("call-with-current-continuation", [&](auto&&... operands)
    {
      return call_cc(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("lambda", [&](auto&&... operands)
    {
      return lambda(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("environment", [&](auto&&... operands)
    {
      return abstraction(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("set!", [&](auto&&... operands)
    {
      return assignment(std::forward<decltype(operands)>(operands)...);
    });

    /*
     * <importation> = (import <library name>)
     */
    define<syntax>("import", [&](auto&& expression,
                                 auto&& lexical_environment,
                                 auto&& continuation, auto)
    {
      using meevax::kernel::path;

      if (const object& library_name {car(expression)}; not library_name)
      {
        throw syntax_error {"empty library-name is not allowed"};
      }
      else if (const object& library {locate_library(library_name)}; library)
      {
        DEBUG_COMPILE_SYNTAX(library_name << " => found library " << library_name << " in this environment as " << library << std::endl);

        /*
         * Passing the VM instruction to load literally library-name as
         * continuation is for return value of this syntax form "import".
         */
        // return import_library(library, cons(_load_literal_, library_name, continuation));
        return import_(library, cons(_load_literal_, library_name, continuation));
      }
      else // XXX Don't use this library style (deprecated)
      {
        DEBUG_COMPILE_SYNTAX("(\t; => unknown library-name" << std::endl);
        NEST_IN;

        /*
         * Macro expander for to evaluate library-name on operand compilation
         * rule.
         */
        environment macro {
          unit,
          interaction_environment()
        }; // TODO IN CONSTRUCTOR INITIALIZATION

        path library_path {""};

        for (const object& identifier : macro.execute(operand(library_name, lexical_environment, continuation)))
        {
          if (identifier.is<path>())
          {
            library_path /= identifier.as<path>();
          }
          else if (identifier.is<string>())
          {
            library_path /= path {identifier.as<string>()};
          }
          else
          {
            throw syntax_error {
              identifier, " is not allowed as part of library-name (must be path or string object)"
            };
          }
        }

        NEST_OUT_SYNTAX;

        std::cerr << "; import\t; dynamic-link " << library_path << std::endl;

        // TODO ADD FUNCTION CALL OPERATOR TO LINKER
        // const object& linker {make<posix::linker>(library_path.c_str())};

        // machine<environment>::define(
        //   library_name,
        //   std::invoke(
        //     dynamic_link(library_path).link<native::signature>("library"),
        //     unit
        //   )
        // );

        const object exported {std::invoke(
          dynamic_link(library_path).link<native::signature>("library"),
          // linker.as<posix::linker>().link<native::signature>("library"),
          unit // TODO PASS SOMETHING USEFUL TO LIBRARY INITIALIZER
        )};

        /*
         * Passing the VM instruction to load literally library-name as
         * continuation is for return value of this syntax form "import".
         */
        auto decralations {import_library(
           exported, cons(_load_literal_, library_name, continuation)
         )};
        // return import_library(
        //    locate_library(library_name),
        //    cons(_load_literal_, library_name, continuation)
        //  );

        /*
         * Push VM instruction for define the library exported from
         * shared-object as given library-name (this will execute on first of VM
         * instruction which result of this function).
         */
        return decralations.push(_load_literal_, exported, _define_, library_name);
      }
    });

    define<native>("load", [&](const object& operands)
    {
      return load(car(operands).as<const string>());
    });

    define<native>("linker", [&](auto&& operands)
    {
      if (auto size {length(operands)}; size < 1)
      {
        throw evaluation_error {
          "procedure linker expects a string for argument, but received nothing."
        };
      }
      else if (const object& s {car(operands)}; not s.is<string>())
      {
        throw evaluation_error {
          "procedure linker expects a string for argument, but received ",
          meevax::utility::demangle(s.type()),
          " rest ", size, " argument",
          (size < 2 ? " " : "s "),
          "were ignored."
        };
      }
      else
      {
        return make<meevax::posix::linker>(s.template as<string>());
      }
    });

    define<native>("native", [&](const iterator& operands)
    {
      // if (auto size {length(operands)}; size < 1)
      // {
      //   throw evaluation_error {
      //     "procedure link expects two arguments (linker and string), but received nothing."
      //   };
      // }
      // else if (size < 2)
      // {
      //   throw evaluation_error {
      //     "procedure link expects two arguments (linker and string), but received only one argument."
      //   };
      // }
      // else if (const auto& linker {car(operands)}; not linker.template is<meevax::posix::linker>())
      // {
      //   throw evaluation_error {
      //     "procedure dynamic-link-open expects a linker for first argument, but received ",
      //     meevax::utility::demangle(linker.type()),
      //     " rest ", size - 1, " argument",
      //     (size < 2 ? " " : "s "),
      //     "were ignored."
      //   };
      // }
      // else if (const auto& name {cadr(operands)}; not name.template is<string>())
      // {
      //   throw evaluation_error {
      //     "procedure dynamic-link-open expects a string for second argument, but received ",
      //     meevax::utility::demangle(name.type()),
      //     " rest ", size - 2, " argument",
      //     (size < 3 ? " " : "s "),
      //     "were ignored."
      //   };
      // }
      // else
      // {
        const std::string name {cadr(operands).as<string>()};

        return make<native>(
          name,
          car(operands).as<posix::linker>().link<native::signature>(name)
        );
      // }
    });

    define<native>("read", [&](const iterator& operands)
    {
      return read(operands ? car(operands).as<input_file>() : std::cin);
    });

    define<native>("write", [&](const iterator& operands)
    {
      std::cout << car(operands);
      return unspecified;
    });

    define<native>("evaluate", [&](auto&& operands)
    {
      return evaluate(car(operands));
    });
  } // environment class default constructor

  template <>
  environment::environment(std::integral_constant<int, 1>)
    : environment::environment {layer<0>}
  {
    static const std::string layer_1 {
      &_binary_layer_1_ss_start, &_binary_layer_1_ss_end
    };
    std::cerr << layer_1 << std::endl;

    std::stringstream stream {
      // #include <meevax/library/r7rs.xss>
      layer_1
    };

    std::size_t loaded {0};

    for (auto e {read(stream)}; e != characters.at("end-of-file"); e = read(stream))
    {
      // if (verbose == true_object or verbose_environment == true_object)
      // {
        std::cerr << "; layer 1\t; " << loaded << " expression loaded";
      // }

      evaluate(e);

      // if (verbose == true_object or verbose_environment == true_object)
      // {
        ++loaded;
        std::cerr << "\r" << std::flush;
      // }
    }

    // if (verbose == true_object or verbose_environment == true_object)
    // {
      std::cerr << std::endl;
    // }
  }

  std::ostream& operator<<(std::ostream& os, const environment& environment)
  {
    return os << highlight::syntax << "#("
              << highlight::constructor << "environemnt"
              << attribute::normal << highlight::comment << " #;" << &environment << attribute::normal
              << highlight::syntax << ")"
              << attribute::normal;
  }
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_ENVIRONMENT_HPP

