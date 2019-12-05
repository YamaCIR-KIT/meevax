#ifndef INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP
#define INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP

#include <algorithm> // std::equal
#include <numeric> // std::accumulate

// Global configuration generated by CMake before compilation.
#include <meevax/kernel/configurator.hpp>
#include <meevax/kernel/machine.hpp>
#include <meevax/kernel/reader.hpp>
#include <meevax/kernel/file.hpp>
#include <meevax/posix/linker.hpp>

/* ==== Embedded Source Codes =================================================
*
* library/layer-1.ss
*
* MEMO: readelf -a layer-1.ss.o
*
*============================================================================ */
extern char _binary_layer_1_ss_start;
extern char _binary_layer_1_ss_end;

namespace meevax::kernel
{
  /* ==== Standard Environment Layers =========================================
  *
  * Layer 0 - Pure Syntax
  * Layer 1 - Derived Expressions and Standard Procedures
  *
  *========================================================================== */
  template <int Layer>
  static constexpr std::integral_constant<int, Layer> layer {};

  class syntactic_continuation
    /* ========================================================================
    *
    * The syntactic-continuation is a pair of "the program" and "global
    * environment (simple association list)". It also has the aspect of a
    * meta-closure that closes the global environment when it constructed (this
    * feature is known as syntactic-closure).
    *
    *======================================================================== */
    : public virtual pair

    /* ========================================================================
    *
    * Reader access symbol table of this syntactic_continuation (by member
    * function "intern") via static polymorphism. The syntactic_continuation
    * indirectly inherits the non-copyable class std::istream (reader base
    * class), so it cannot be copied.
    *
    *======================================================================== */
    , public reader<syntactic_continuation>

    /* ========================================================================
    *
    * Each syntactic-continuation has a virtual machine and a compiler.
    *
    *======================================================================== */
    , public machine<syntactic_continuation>

    /* ========================================================================
    *
    * Global configuration is shared in all of syntactic_continuations running
    * on same process. Thus, any change of configuration member influences any
    * other syntactic_continuations immediately.
    *
    *======================================================================== */
    , public configurator<syntactic_continuation>
  {
    // TODO Rename to internal_symbols
    std::unordered_map<std::string, object> symbols;

    // TODO expand if external_symbols empty
    std::unordered_map<object, object> external_symbols;

  public: // Constructors
    // for bootstrap scheme-report-environment
    template <int Layer>
    explicit syntactic_continuation(std::integral_constant<int, Layer>);

    // for library constructor
    template <typename... Ts>
    explicit syntactic_continuation(Ts&&... operands)
      : pair {std::forward<decltype(operands)>(operands)...}
    {}

  public: // Interfaces
    // TODO Rename to "interaction_ready"
    auto ready() const noexcept
    {
      return reader<syntactic_continuation>::ready();
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

    template <typename T, typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return
        machine<syntactic_continuation>::define(
          intern(name),
          make<T>(
            name,
            std::forward<decltype(operands)>(operands)...));
    }

    template <typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return
        machine<syntactic_continuation>::define(
          intern(name),
          std::forward<decltype(operands)>(operands)...);
    }

    std::size_t time_stamp {0};

    const auto& rename(const object& object)
    {
      if (not object.is<symbol>())
      {
        if (verbose == true_object or verbose_environment == true_object)
        {
          std::cerr << "; auto-rename\t; ignored " << object << std::endl;
        }

        return object;
      }
      else
      {
        // XXX TIME STAMP REQUIRED???
        // const std::string name {
        //   object.as<const std::string>() + "." + std::to_string(time_stamp)
        // };

        if (verbose == true_object or verbose_environment == true_object)
        {
          // std::cerr << "; auto-rename\t; renaming " << object << " => " << name << std::endl;
          std::cerr << "; auto-rename\t; renaming " << object << std::endl;
        }

        return intern(object.as<symbol>());
        // return intern(name);
      }
    }

    // decltype(auto) export_(const object& key, const object& value)
    // {
    //   if (auto iter {bindings.find(key)}; iter != std::end(bindings))
    //   {
    //     if (verbose == true_object or verbose_environment == true_object)
    //     {
    //       std::cerr << "; export\t; detected redefinition (interactive mode ignore previous definition)" << std::endl;
    //     }
    //
    //     bindings.at(key) = value;
    //     return bindings.find(key);
    //   }
    //   else
    //   {
    //     if (verbose == true_object or verbose_environment == true_object)
    //     {
    //       std::cerr << "; export\t; exporting new binding (" << key << " . " << value << ")" << std::endl;
    //     }
    //
    //     return bindings.emplace(key, value).first;
    //   }
    // }

    decltype(auto) continuation()
    {
      return std::get<0>(*this);
    }

    decltype(auto) current_expression()
    {
      return car(continuation());

      // if (auto& k {continuation()}; not k)
      // {
      //   return k;
      // }
      // else
      // {
      //   return car(k);
      // }
    }

    decltype(auto) lexical_environment()
    {
      return cdr(continuation());

      // if (auto& k {continuation()}; not k)
      // {
      //   return k;
      // }
      // else
      // {
      //   return cdr(k);
      // }
    }

    decltype(auto) interaction_environment() noexcept
    {
      return static_cast<stack&>(second);
    }

    decltype(auto) expand(const object& operands)
    {
      // TODO execute following if the staged changes is not empty

      // std::cerr << "; macroexpand\t; " << operands << std::endl;

      ++time_stamp;

      s = unit;
      // std::cerr << ";\t\t; s = " << s << std::endl;

      e = cons(operands, lexical_environment());
      // std::cerr << ";\t\t; e = " << e << std::endl;

      c = current_expression();
      // std::cerr << ";\t\t; c = " << c << std::endl;

      d = cons(
            unit,                                    // s
            unit,                                    // e
            list(make<instruction>(mnemonic::STOP)), // c
            unit);                                   // d
      // std::cerr << ";\t\t; d = " << d << std::endl;

      const auto result {execute()};
      // std::cerr << "; \t\t; " << result << std::endl;
      return result;
    }

    template <typename... Ts>
    decltype(auto) evaluate(Ts&&... operands)
    {
      return execute(compile(std::forward<decltype(operands)>(operands)...));
    }

    // const auto& dynamic_link(const std::string& path)
    // {
    //   if (auto iter {linkers.find(path)}; iter != std::end(linkers))
    //   {
    //     return iter->second;
    //   }
    //   else
    //   {
    //     iter = linkers.emplace(path, path).first;
    //     return iter->second;
    //   }
    // }

    [[deprecated]]
    auto locate_library(const object& name)
    {
      for (const object& each : interaction_environment())
      {
        if (const object& key {car(each)}; not key.is<symbol>())
        {
          if (recursively_equivalent(key, name))
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

      assert(library.is<syntactic_continuation>());

      for (const object& each : library.as<syntactic_continuation>().interaction_environment())
      {
        executable.push(
          make<instruction>(mnemonic::LOAD_LITERAL), cadr(each),
          make<instruction>(mnemonic::DEFINE), rename(car(each)));
      }

      return executable;
    }

    // auto import_(const object& library, const object& continuation)
    // {
    //   auto& source {library.as<syntactic_continuation>()};
    //
    //   if (source.bindings.empty())
    //   {
    //     source.expand(list(library));
    //
    //     if (source.bindings.empty())
    //     {
    //       throw syntax_error {library, " is may not be library"};
    //     }
    //   }
    //
    //   stack executable {continuation};
    //
    //   for (const auto& [key, value] : source.bindings)
    //   {
    //     executable.push(_load_literal_, value, make<instruction>(mnemonic::DEFINE), rename(key));
    //   }
    //
    //   return executable;
    // }

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

  public: // experimental
    // TODO Rename
    // auto reference(const object& identifier)
    // {
    //   std::cerr << "; reference\t; " << identifier << std::endl;
    //   for (const object& commit : interaction_environment())
    //   {
    //     if (recursively_equivalent(car(commit), identifier))
    //     {
    //       std::cerr << ";\t\t; found: " << commit << std::endl;
    //       return cadr(commit);
    //     }
    //   }
    //
    //   return identifier;
    // }
  };

  template <>
  syntactic_continuation::syntactic_continuation(std::integral_constant<int, 0>)
  {
    define<special>("quote", [&](auto&&... operands)
    {
      return quotation(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("if", [&](auto&&... operands)
    {
      return conditional(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("define", [&](auto&&... operands)
    {
      return definition(std::forward<decltype(operands)>(operands)...);
    });

    // TODO Rename to "sequential"
    define<special>("begin", [&](auto&&... operands)
    {
      return sequence(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("call-with-current-continuation", [&](auto&&... operands)
    {
      return call_cc(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("lambda", [&](auto&&... operands)
    {
      return lambda(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("fork-with-current-syntactic-continuation", [&](auto&&... operands)
    {
      return fork(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("call-with-current-syntactic-continuation", [&](auto&&... operands)
    {
      return call_csc(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("set!", [&](auto&&... operands)
    {
      return assignment(std::forward<decltype(operands)>(operands)...);
    });

    define<special>("reference", [&](auto&&... operands)
    {
      return reference(std::forward<decltype(operands)>(operands)...);
    });

    /*
     * <importation> = (import <library name>)
     */
    // define<special>("import", [&](auto&& expression,
    //                              auto&& lexical_environment,
    //                              auto&& continuation, auto)
    // {
    //   using meevax::kernel::path;
    //
    //   if (const object& library_name {car(expression)}; not library_name)
    //   {
    //     throw syntax_error {"empty library-name is not allowed"};
    //   }
    //   else if (const object& library {locate_library(library_name)}; library)
    //   {
    //     DEBUG_COMPILE_SYNTAX(library_name << " => found library " << library_name << " in this environment as " << library << std::endl);
    //
    //     /*
    //      * Passing the VM instruction to load literally library-name as
    //      * continuation is for return value of this syntax form "import".
    //      */
    //     // return import_library(library, cons(_load_literal_, library_name, continuation));
    //     return import_(library, cons(_load_literal_, library_name, continuation));
    //   }
    //   else // XXX Don't use this library style (deprecated)
    //   {
    //     DEBUG_COMPILE_SYNTAX("(\t; => unknown library-name" << std::endl);
    //     NEST_IN;
    //
    //     /**********************************************************************
    //     * Macro expander for to evaluate library-name on operand compilation
    //     * rule.
    //     **********************************************************************/
    //     syntactic_continuation macro {
    //       unit,
    //       interaction_environment()
    //     }; // TODO IN CONSTRUCTOR INITIALIZATION
    //
    //     path library_path {""};
    //
    //     for (const object& identifier : macro.execute(operand(library_name, lexical_environment, continuation)))
    //     {
    //       if (identifier.is<path>())
    //       {
    //         library_path /= identifier.as<path>();
    //       }
    //       else if (identifier.is<string>())
    //       {
    //         library_path /= path {identifier.as<string>()};
    //       }
    //       else
    //       {
    //         throw syntax_error {
    //           identifier, " is not allowed as part of library-name (must be path or string object)"
    //         };
    //       }
    //     }
    //
    //     NEST_OUT_SYNTAX;
    //
    //     std::cerr << "; import\t; dynamic-link " << library_path << std::endl;
    //
    //     // TODO ADD FUNCTION CALL OPERATOR TO LINKER
    //     // const object& linker {make<posix::linker>(library_path.c_str())};
    //
    //     // machine<syntactic_continuation>::define(
    //     //   library_name,
    //     //   std::invoke(
    //     //     dynamic_link(library_path).link<procedure::signature>("library"),
    //     //     unit
    //     //   )
    //     // );
    //
    //     const object exported {std::invoke(
    //       dynamic_link(library_path).link<procedure::signature>("library"),
    //       // linker.as<posix::linker>().link<procedure::signature>("library"),
    //       unit // TODO PASS SOMETHING USEFUL TO LIBRARY INITIALIZER
    //     )};
    //
    //     /*
    //      * Passing the VM instruction to load literally library-name as
    //      * continuation is for return value of this syntax form "import".
    //      */
    //     auto decralations {import_library(
    //        exported, cons(_load_literal_, library_name, continuation)
    //      )};
    //     // return import_library(
    //     //    locate_library(library_name),
    //     //    cons(_load_literal_, library_name, continuation)
    //     //  );
    //
    //     /**********************************************************************
    //     * Push VM instruction for define the library exported from
    //     * shared-object as given library-name (this will execute on first of VM
    //     * instruction which result of this function).
    //     **********************************************************************/
    //     return decralations.push(_load_literal_, exported, make<instruction>(mnemonic::DEFINE), library_name);
    //   }
    // });

    define<procedure>("load", [&](auto&&, auto&& operands)
    {
      return load(car(operands).template as<const string>());
    });

    define<procedure>("linker", [&](auto&&, auto&& operands)
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
        return make<meevax::posix::linker>(s.as<string>());
      }
    });

    define<procedure>("procedure-from", [&](auto&&, auto&& operands)
    {
      const std::string name {cadr(operands).template as<string>()};

      return
        make<procedure>(
          name,
          car(operands).template as<posix::linker>().template link<procedure::signature>(name));
    });

    define<procedure>("read", [&](auto&&, auto&& operands)
    {
      return read(operands ? car(operands).template as<input_file>() : std::cin);
    });

    define<procedure>("write", [&](auto&&, auto&& operands)
    {
      std::cout << car(operands);
      return unspecified;
    });

    define<procedure>("evaluate", [&](auto&&, auto&& operands)
    {
      return evaluate(car(operands));
    });

    define<procedure>("compile", [&](auto&&, auto&& operands)
    {
      return compile(car(operands));
    });
  } // syntactic_continuation class default constructor

  template <>
  syntactic_continuation::syntactic_continuation(std::integral_constant<int, 1>)
    : syntactic_continuation::syntactic_continuation {layer<0>}
  {
    static const std::string layer_1 {
      &_binary_layer_1_ss_start, &_binary_layer_1_ss_end
    };
    // std::cerr << layer_1 << std::endl;

    std::stringstream stream {layer_1};

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

  std::ostream& operator<<(std::ostream& os, const syntactic_continuation& syntactic_continuation)
  {
    return os << highlight::syntax << "#("
              << highlight::constructor << "syntactic-continuation"
              << attribute::normal << highlight::comment << " #;" << &syntactic_continuation << attribute::normal
              << highlight::syntax << ")"
              << attribute::normal;
  }
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP

