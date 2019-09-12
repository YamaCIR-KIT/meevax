#ifndef INCLUDED_MEEVAX_SYSTEM_ENVIRONMENT_HPP
#define INCLUDED_MEEVAX_SYSTEM_ENVIRONMENT_HPP

#include <algorithm> // std::equal
#include <numeric> // std::accumulate

/**
 * Global configuration generated by CMake before compilation.
 */
#include <meevax/system/configurator.hpp>
#include <meevax/system/machine.hpp>
#include <meevax/system/reader.hpp>

#include <meevax/posix/linker.hpp>

namespace meevax::system
{
  template <int Version>
  static constexpr std::integral_constant<int, Version> standard_environment {};

  class environment
    /*
     * The environment is a pair of "current expression" and "global environment
     * (simple association list)". It also has the aspect of a meta-closure that
     * closes the global environment when it constructed (this feature is known
     * as syntactic-closure).
     */
    : public closure

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

    static inline std::unordered_map<std::string, posix::linker> linkers {};

  public: // Constructors
    // for macro
    environment() = default;

    // for bootstrap scheme-report-environment
    template <int Version>
    environment(std::integral_constant<int, Version>);

    // for library constructor
    template <typename... Ts>
    constexpr environment(Ts&&... args)
      : pair {std::forward<Ts>(args)...} // virtual base of closure
    {}

  public: // Module System Interface
    auto ready() const noexcept
    {
      return reader<environment>::ready();
    }

    template <typename T, typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... args)
    {
      return system::machine<environment>::define(
               intern(name), make<T>(name, std::forward<Ts>(args)...)
             );
    }

    template <typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... xs)
    {
      return system::machine<environment>::define(
               intern(name), std::forward<Ts>(xs)...
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
        throw error {"internal unacceptable renaming"};
      }
      else
      {
        return intern(object.as<symbol>());
      }
    }

    decltype(auto) current_expression() noexcept
    {
      return std::get<0>(*this);
    }

    decltype(auto) interaction_environment() noexcept
    {
      return static_cast<stack&>(std::get<1>(*this));
    }

    decltype(auto) expand(const object& operands)
    {
      std::cerr << "macroexpand " << operands << std::endl;

      s = unit;
      e = list(operands);
      c = current_expression();
      d = cons(
            unit,         // s
            unit,         // e
            list(_stop_), // c
            unit          // d
          );

      return execute();
    }

  public: // Library System Interfaces
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

    auto import_library(const object& library, const object& continuation)
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

  public: // deprecated
    template <typename... Ts>
    [[deprecated]] decltype(auto) load(Ts&&... args)
    {
      const std::string path {std::forward<Ts>(args)...};

      const auto checkpoint {interaction_environment()};

      if (reader<environment> port {path}; port)
      {
        std::swap(*this, port);

        d.push(s, e, c);
        s = e = c = unit;

        while (ready()) try
        {
          const auto expression {read()};
          // std::cerr << "[loader] expression: " << expression << std::endl;
          const auto executable {compile(expression)};
          // std::cerr << "[loader] executable: " << executable << std::endl;
          const auto evaluation {execute(executable)};
          // std::cerr << "[loader] evaluation: " << evaluation << std::endl;
        }
        catch (...)
        {
          interaction_environment() = checkpoint;
          std::cerr << "[error] failed to load \"" << path << "\" with no-error; reverted changes for interaction-environment (exclude side-effects)." << std::endl;
          std::swap(*this, port);
          throw;
        }

        std::swap(*this, port);
        std::cerr << "; load  \t; " << std::distance(interaction_environment(), checkpoint) << " expression defined" << std::endl;

        s = d.pop();
        e = d.pop();
        c = d.pop();

        return interaction_environment();
      }
      else
      {
        throw error {"failed to open file \"", path, "\""};
      }
    }
  };

  template <>
  environment::environment(std::integral_constant<int, 0>)
  {
    define<syntax>("quote", [&](auto&&... xs)
    {
      return quotation(std::forward<decltype(xs)>(xs)...);
    });

    define<syntax>("if", [&](auto&& expression, auto&& lexical_environment, auto&& continuation, auto tail)
    {
      TRACE("compile") << car(expression) << " ; => is <test>" << std::endl;

      if (tail)
      {
        const auto consequent {compile(cadr(expression), lexical_environment, list(_return_), true)};

        const auto alternate {
          cddr(expression) ? compile(caddr(expression), lexical_environment, list(_return_), true)
                           : list(_load_literal_, unspecified, _return_)
        };

        return compile(
                 car(expression), // <test>
                 lexical_environment,
                 cons(_select_tail_, consequent, alternate, cdr(continuation))
               );
      }
      else
      {
        const auto consequent {compile(cadr(expression), lexical_environment, list(_join_))};

        const auto alternate {
          cddr(expression) ? compile(caddr(expression), lexical_environment, list(_join_))
                           : list(_load_literal_, unspecified, _join_)
        };

        return compile(
                 car(expression), // <test>
                 lexical_environment,
                 cons(_select_, consequent, alternate, continuation)
               );
      }
    });

    define<syntax>("define", [&](auto&&... operands)
    {
      return definition(std::forward<decltype(operands)>(operands)...);
    });

    define<syntax>("begin", [&](auto&&... args)
    {
      return sequence(std::forward<decltype(args)>(args)...);
    });

    define<syntax>("call-with-current-continuation", [&](auto&& expression, auto&& lexical_environment, auto&& continuation, auto)
    {
      TRACE("compile") << car(expression) << " ; => is <procedure>" << std::endl;

      return cons(
               _make_continuation_,
               continuation,
               compile(
                 car(expression),
                 lexical_environment,
                 cons(_apply_, continuation)
               )
             );
    });

    define<syntax>("lambda", [&](auto&& expression, auto&& lexical_environment, auto&& continuation, auto)
    {
      TRACE("compile") << car(expression) << " ; => is <formals>" << std::endl;

      return cons(
               _make_closure_,
               body(
                 cdr(expression), // <body>
                 cons(car(expression), lexical_environment), // extend lexical environment
                 list(_return_) // continuation of body (finally, must be return)
               ),
               continuation
             );
    });

    define<syntax>("environment", [&](auto&& expression,
                                       auto&& lexical_environment,
                                       auto&& continuation, auto)
    {
      TRACE("compile") << car(expression) << "\t; => <formals>" << std::endl;

      return cons(
               _make_environment_,
               program(
                 cdr(expression),
                 cons(car(expression), lexical_environment),
                 list(_return_)
               ),
               continuation
             );
    });

    define<syntax>("set!", [&](auto&&... args)
    {
      return set(std::forward<decltype(args)>(args)...);
    });

    /*
     * <importation> = (import <library name>)
     */
    define<syntax>("import", [&](auto&& expression,
                                  auto&& lexical_environment,
                                  auto&& continuation, auto)
    {
      using meevax::system::path;

      if (const object& library_name {car(expression)}; not library_name)
      {
        throw error {"empty library-name is not allowed"};
      }
      else if (const object& library {locate_library(library_name)}; library)
      {
        TRACE("compile") << library_name << " => found library " << library_name << " in this environment as " << library << std::endl;

        /*
         * Passing the VM instruction to load literally library-name as
         * continuation is for return value of this syntax form "import".
         */
        return import_library(library, cons(_load_literal_, library_name, continuation));
      }
      else
      {
        TRACE("compile") << "(\t; => unknown library-name" << std::endl;
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
            throw error {identifier, " is not allowed as part of library-name (must be path or string object)"};
          }
        }

        NEST_OUT;

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

    define<native>("load", [&](const object& args)
    {
      return load(car(args).as<const string>());
    });

    define<native>("symbol", [&](const object& args)
    {
      try
      {
        return make<symbol>(car(args).as<string>());
      }
      catch (...)
      {
        return make<symbol>();
      }
    });

    define<native>("linker", [&](auto&& args)
    {
      if (auto size {length(args)}; size < 1)
      {
        throw error {"procedure linker expects a string for argument, but received nothing."};
      }
      else if (const object& s {car(args)}; not s.is<string>())
      {
        throw error {
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

    define<native>("link", [&](auto&& args)
    {
      if (auto size {length(args)}; size < 1)
      {
        throw error {"procedure link expects two arguments (linker and string), but received nothing."};
      }
      else if (size < 2)
      {
        throw error {"procedure link expects two arguments (linker and string), but received only one argument."};
      }
      else if (const auto& linker {car(args)}; not linker.template is<meevax::posix::linker>())
      {
        throw error {
                "procedure dynamic-link-open expects a linker for first argument, but received ",
                meevax::utility::demangle(linker.type()),
                " rest ", size - 1, " argument",
                (size < 2 ? " " : "s "),
                "were ignored."
              };
      }
      else if (const auto& name {cadr(args)}; not name.template is<string>())
      {
        throw error {
                "procedure dynamic-link-open expects a string for second argument, but received ",
                meevax::utility::demangle(name.type()),
                " rest ", size - 2, " argument",
                (size < 3 ? " " : "s "),
                "were ignored."
              };
      }
      else
      {
        const auto& linker_ {car(args).template as<meevax::posix::linker>()};
        const std::string& name_ {cadr(args).template as<string>()};
        return make<native>(
                 name_,
                 linker_.template link<typename native::signature>(name_)
               );
      }
    });
  } // environment class default constructor

  std::ostream& operator<<(std::ostream& os, const environment& environment)
  {
    return os << highlight::syntax << "#("
              << highlight::constructor << "environemnt"
              << attribute::normal << highlight::comment << " #;" << &environment << attribute::normal
              << highlight::syntax << ")"
              << attribute::normal;
  }
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_ENVIRONMENT_HPP

