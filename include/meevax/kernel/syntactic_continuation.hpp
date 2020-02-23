#ifndef INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP
#define INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP

#include <algorithm> // std::equal

#include <meevax/kernel/configurator.hpp>
#include <meevax/kernel/linker.hpp>
#include <meevax/kernel/machine.hpp>
#include <meevax/kernel/reader.hpp>

/* ==== Embedded Source Codes ==================================================
*
* library/hoge.ss
*
* MEMO: readelf -a hoge.ss.o
*
*============================================================================ */
extern char _binary_overture_ss_start;
extern char _binary_overture_ss_end;

namespace meevax::kernel
{
  /* ==== Standard Environment Layers ==========================================
  *
  * Layer 0 - Program Structure Controllers
  * Layer 1 - Primitive Expression Types
  * Layer 2 - Derived Expression Types and Standard Procedures
  *
  *========================================================================== */
  template <int Layer>
  static constexpr std::integral_constant<int, Layer> layer {};

  class syntactic_continuation
    /* =========================================================================
    *
    * The syntactic-continuation is a pair of "the program" and "global
    * environment (simple association list)". It also has the aspect of a
    * meta-closure that closes the global environment when it constructed (this
    * feature is known as syntactic-closure).
    *
    *======================================================================== */
    : public virtual pair

    /* =========================================================================
    *
    * Reader access symbol table of this syntactic_continuation (by member
    * function "intern") via static polymorphism. The syntactic_continuation
    * indirectly inherits the non-copyable class std::istream (reader base
    * class), so it cannot be copied.
    *
    *======================================================================== */
    , public reader<syntactic_continuation>

    /* =========================================================================
    *
    * Each syntactic-continuation has a virtual machine and a compiler.
    *
    *======================================================================== */
    , public machine<syntactic_continuation>

    /* =========================================================================
    *
    * Global configuration is shared in all of syntactic_continuations running
    * on same process. Thus, any change of configuration member influences any
    * other syntactic_continuations immediately.
    *
    *======================================================================== */
    , public configurator<syntactic_continuation>
  {
  public: // TODO PRIVATE
    std::unordered_map<std::string, object> symbols;
    std::unordered_map<std::string, object> external_symbols;

    std::size_t current_layer {0};

    bool virgin {true};

    std::size_t experience {0};

  public: // Accessors
    const auto& program() const
    {
      return first;
    }

    auto& interaction_environment()
    {
      return second;
    }

    decltype(auto) current_expression()
    {
      return car(program());
    }

    decltype(auto) lexical_environment()
    {
      return cdr(program());
    }

  public: // Constructors
    template <typename... Ts>
    explicit syntactic_continuation(Ts&&... operands)
      : pair {std::forward<decltype(operands)>(operands)...}
    {
      boot(layer<0>);

      if (first)
      {
        s = car(first);
        e = cadr(first);
        c = unit;
        d = cdddr(first);

        const auto subprogram {compile(
          car(caddr(first)),
          interaction_environment(), // syntactic-environment
          cdr(caddr(first)),
          list(
            make<instruction>(mnemonic::STOP)),
          as_program_declaration
        )};

        c = subprogram;

        // std::cerr << ";\t\t; s = " << s << std::endl;
        // std::cerr << ";\t\t; e = " << e << std::endl;
        // std::cerr << ";\t\t; c = " << c << std::endl;
        // std::cerr << ";\t\t; d = " << d << std::endl;

        first = execute();

        // std::cerr << ";\t\t; s = " << s << std::endl;
        // std::cerr << ";\t\t; e = " << e << std::endl;
        // std::cerr << ";\t\t; c = " << c << std::endl;
        // std::cerr << ";\t\t; d = " << d << std::endl;

        assert(first.is<closure>());
      }
    }

    template <auto N>
    explicit syntactic_continuation(std::integral_constant<decltype(N), N>);

    template <auto N>
    void boot(std::integral_constant<decltype(N), N>);

    // template <auto N>
    // void boot_up_to(std::integral_constant<decltype(N), N>)
    // {
    // }

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
        return (*iter).second;
      }
      else
      {
        const auto [position, success] {symbols.emplace(s, make<symbol>(s))};
        assert(success);
        return (*position).second;
      }
    }

    const auto& override(const object& identifier, const object& environment)
    {
      if (not identifier or not environment)
      {
        return identifier;
      }
      else if (caar(environment).equivalent_to(identifier))
      {
        return caar(environment);
      }
      else
      {
        return override(identifier, cdr(environment));
      }
    }

    template <typename T, typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return
        machine<syntactic_continuation>::define(
          override(
            intern(name),
            interaction_environment()),
          // intern(name),
          make<T>(
            name,
            std::forward<decltype(operands)>(operands)...));
    }

    template <typename... Ts>
    decltype(auto) define(const std::string& name, Ts&&... operands)
    {
      return
        machine<syntactic_continuation>::define(
          override(
            intern(name),
            interaction_environment()),
          // intern(name),
          std::forward<decltype(operands)>(operands)...);
    }

    const auto& rename(const object& object)
    {
      if (not object.is<symbol>())
      {
        if (verbose.equivalent_to(t))
        {
          std::cerr << "; package\t; renamer ignored non-symbol object "
                    << object
                    << std::endl;
        }

        return object;
      }
      else
      {
        const std::string name {
          object.as<const std::string>() + "." + std::to_string(experience)
        };

        if (verbose.equivalent_to(t))
        {
          std::cerr << "; package\t; renaming \"" << object << "\" to \"" << name << "\"" << std::endl;
          // std::cerr << "; package\t; renaming " << object << std::endl;
        }

        // return intern(object.as<symbol>());
        return intern(name);
      }
    }

    decltype(auto) expand(const object& operands)
    {
      // std::cerr << "; macroexpand\t; " << operands << std::endl;

      push( // XXX ???
        d,
        s,
        e,
        cons(
          make<instruction>(mnemonic::STOP),
          c));

      s = unit;

      e = cons(
            operands, // <lambda> parameters
            lexical_environment()); // static environment

      c = current_expression();

      // std::cerr << ";\t\t; s = " << s << std::endl;
      // std::cerr << ";\t\t; e = " << e << std::endl;
      // std::cerr << ";\t\t; c = " << c << std::endl;
      // std::cerr << ";\t\t; d = " << d << std::endl;

      const auto result {execute()};
      // std::cerr << "; \t\t; " << result << std::endl;

      virgin = false;
      ++experience;

      return result;
    }

    // TODO CONVERT TO VM INSTRUCTION
    decltype(auto) evaluate(const object& expression)
    {
      return
        execute_interrupt(
          compile(
            expression,
            interaction_environment()));
    }

    template <typename... Ts>
    decltype(auto) load(Ts&&... operands)
    {
      const std::string path {std::forward<decltype(operands)>(operands)...};

      if (verbose.equivalent_to(t))
      {
        std::cerr << "; loader\t; open \"" << path << "\" => ";
      }

      if (std::fstream stream {path}; stream)
      {
        if (verbose.equivalent_to(t))
        {
          std::cerr << "succeeded" << std::endl;
        }

        push(d, s, e, c);
        s = e = c = unit;

        for (auto e {read(stream)}; e != characters.at("end-of-file"); e = read(stream))
        {
          if (verbose.equivalent_to(t))
          {
            std::cerr << "; read\t\t; " << e << std::endl;
          }

          evaluate(e);
        }

        s = pop(d);
        e = pop(d);
        c = pop(d);

        return unspecified;
      }
      else
      {
        if (verbose.equivalent_to(t))
        {
          std::cerr << "failed" << std::endl;
        }

        throw evaluation_error {"failed to open file ", std::quoted(path)};
      }
    }

  public: // Primitive Expression Types
    DEFINE_PRIMITIVE_EXPRESSION(exportation,
    {
      if (verbose.equivalent_to(t))
      {
        std::cerr
        << (not depth ? "; compile\t; " : ";\t\t; ")
        << std::string(depth * 2, ' ')
        << expression
        << highlight::comment << " is <export specs>"
        << attribute::normal << std::endl;
      }

      auto exportation = [this](auto&&, const object& operands)
      {
        for (const auto& each : operands)
        {
          std::cerr << ";\t\t; staging " << each << std::endl;

          external_symbols.emplace(
            write(std::stringstream {}, each).str(),
            each);
        }

        std::cerr << ";\t\t; exported identifiers are" << std::endl;

        for (const auto& [key, value] : external_symbols)
        {
          assert(not std::empty(key));
          std::cerr << ";\t\t;   " << value << std::endl;
        }

        return unspecified;
      };

      return
        cons(
          make<instruction>(mnemonic::LOAD_CONSTANT), expression,
          make<instruction>(mnemonic::LOAD_CONSTANT), make<procedure>("exportation", exportation),
          make<instruction>(mnemonic::CALL),
          continuation);
    })

    DEFINE_PRIMITIVE_EXPRESSION(importation,
    {
      auto importation = [&](auto&&, const object& operands)
      {
        assert(
          operands.is<syntactic_continuation>());

        if (operands.as<syntactic_continuation>().external_symbols.empty())
        {
          std::cerr << "; import\t; " << operands << " is virgin => expand" << std::endl;
          operands.as<syntactic_continuation>().expand(
            cons(
              operands, unit));
        }

        for (const auto& [key, value] : operands.as<syntactic_continuation>().external_symbols)
        {
          assert(not std::empty(key));
          std::cerr << ";\t\t; importing " << value << std::endl;
        }

        return unspecified;
      };

      return
        reference( // XXX DIRTY HACK
          expression,
          syntactic_environment,
          frames,
          cons(
            make<instruction>(mnemonic::LOAD_CONSTANT), make<procedure>("import", importation),
            make<instruction>(mnemonic::CALL),
            continuation));
    })

  public:
    friend auto operator<<(std::ostream& os, const syntactic_continuation& sc)
      -> decltype(os)
    {
      return os << highlight::syntax << "#("
                << highlight::type << "syntactic-continuation"
                << attribute::normal << highlight::comment << " #;" << &sc << attribute::normal
                << highlight::syntax << ")"
                << attribute::normal;
    }
  };

  #define DEFINE_SPECIAL(NAME, RULE)                                           \
  define<special>(NAME, [this](auto&&... operands)                             \
  {                                                                            \
    return                                                                     \
      RULE(                                                                    \
        std::forward<decltype(operands)>(operands)...);                        \
  })

  #define DEFINE_PROCEDURE_1(NAME, CALLEE)                                     \
  define<procedure>(NAME, [this](auto&&, auto&& operands)                      \
  {                                                                            \
    return                                                                     \
      CALLEE(                                                                  \
        car(operands));                                                        \
  })

  #define DEFINE_PROCEDURE_S(NAME, CALLEE)                                     \
  define<procedure>(NAME, [this](auto&&, auto&& operands)                      \
  {                                                                            \
    return                                                                     \
      CALLEE(                                                                  \
        car(operands).template as<const string>());                            \
  })

  template <>
  void syntactic_continuation::boot(std::integral_constant<decltype(0), 0>)
  {
    // DEFINE_PROCEDURE_1("compile",  compile);

    DEFINE_PROCEDURE_1("evaluate", evaluate);

    define<procedure>("identifier=?", [this](auto&&, auto&& arguments)
    {
      // std::cerr << "; identifier=?\t; car\t; " << car(arguments) << std::endl;
      // std::cerr << ";\t\t; cadr\t; " << cadr(arguments) << std::endl;
      // XXX 二度リネームが発生すると hoge.0.0 見たいな名前になってしまって壊れる
      // return rename(car(arguments)) == rename(cadr(arguments)) ? t : f;
      return car(arguments) == rename(cadr(arguments)) ? t : f;
    });

    DEFINE_SPECIAL("export", exportation);
    DEFINE_SPECIAL("import", importation);
  }

  template <>
  void syntactic_continuation::boot(std::integral_constant<decltype(1), 1>)
  {
    DEFINE_SPECIAL("begin",     sequence);
    DEFINE_SPECIAL("define",    definition);
    DEFINE_SPECIAL("fork",      fork);
    DEFINE_SPECIAL("if",        conditional);
    DEFINE_SPECIAL("lambda",    lambda);
    DEFINE_SPECIAL("quote",     quotation);
    DEFINE_SPECIAL("reference", reference);
    DEFINE_SPECIAL("set!",      assignment);

    DEFINE_SPECIAL("call-with-current-continuation", call_cc);

    DEFINE_PROCEDURE_S("load",   load);
    DEFINE_PROCEDURE_S("linker", make<linker>);

    // define<special>("cons", [this](
    //   auto&& expression,
    //   auto&& syntactic_environment,
    //   auto&& frames,
    //   auto&& continuation,
    //   auto&&)
    // {
    //   return
    //     compile(
    //       cadr(expression),
    //       syntactic_environment,
    //       frames,
    //       compile(
    //         car(expression),
    //         syntactic_environment,
    //         frames,
    //         cons(
    //           make<instruction>(mnemonic::CONS),
    //           continuation)));
    // });

    define<procedure>("features", [this](auto&&...)
    {
      return feature_object;
    });

    define<procedure>("procedure-from", [](auto&&, auto&& operands)
    {
      const std::string name {cadr(operands).template as<string>()};

      return
        make<procedure>(
          name,
          car(operands)
            .template as<linker>()
              .template link<procedure::signature>(name));
    });

    define<procedure>("read", [this](auto&&, auto&& operands)
    {
      return
        read(
          operands ? car(operands).template as<input_port>() : std::cin);
    });

    define<procedure>("write", [](auto&&, auto&& operands)
    {
      std::cout << car(operands);
      return unspecified;
    });
  }

  template <>
  void syntactic_continuation::boot(std::integral_constant<decltype(2), 2>)
  {
    static const std::string overture {
      &_binary_overture_ss_start,
      &_binary_overture_ss_end
    };

    std::stringstream stream {overture};

    std::size_t counts {0};

    for (auto e {read(stream)}; e != characters.at("end-of-file"); e = read(stream))
    {
      std::cerr << "; layer-1\t; "
                << counts++
                << " expression loaded"
                << std::endl;

      evaluate(e);

      static constexpr auto cursor_up {"\x1b[1A"};

      std::cerr << cursor_up << "\r\x1b[K" << std::flush;
    }

    std::cerr << std::endl;
  }

  #undef DEFINE_SPECIAL
  #undef DEFINE_PROCEDURE_1
  #undef DEFINE_PROCEDURE_S

  template <>
  syntactic_continuation::syntactic_continuation(
    std::integral_constant<decltype(1), 1>)
    : syntactic_continuation::syntactic_continuation {}
  {
    boot(layer<1>);
  }

  template <auto N>
  syntactic_continuation::syntactic_continuation(
    std::integral_constant<decltype(N), N>)
    : syntactic_continuation::syntactic_continuation {layer<N - 1>}
  {
    boot(layer<N>);
  }
} // namespace meevax::kernel

#endif // INCLUDED_MEEVAX_KERNEL_SYNTACTIC_CONTINUATION_HPP

