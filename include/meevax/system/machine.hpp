#ifndef INCLUDED_MEEVAX_SYSTEM_MACHINE_HPP
#define INCLUDED_MEEVAX_SYSTEM_MACHINE_HPP

#include <functional> // std::invoke

#include <meevax/system/boolean.hpp> // _false_
#include <meevax/system/closure.hpp>
#include <meevax/system/exception.hpp>
#include <meevax/system/instruction.hpp>
#include <meevax/system/number.hpp>
#include <meevax/system/procedure.hpp>
#include <meevax/system/special.hpp>
#include <meevax/system/srfi-1.hpp> // assoc
#include <meevax/system/symbol.hpp>
#include <meevax/utility/debug.hpp>

#define DEBUG(N) // std::cerr << "; machine\t; " << "\x1B[?7l" << take(c, N) << "\x1B[?7h" << std::endl

namespace meevax::system
{
  template <typename Enclosure>
  class machine // Simple SECD machine.
  {
  protected: // XXX TO PRIVATE
    cursor s, // stack
           e, // lexical environment (rib cage representation)
           c, // control
           d; // dump

  public:
    decltype(auto) interaction_environment()
    {
      return static_cast<Enclosure&>(*this).interaction_environment();
    }

    // Direct virtual machine instruction invocation.
    template <typename... Ts>
    decltype(auto) define(const objective& key, Ts&&... args)
    {
    #if 0
      return interaction_environment().push(list(key, std::forward<Ts>(args)...));
    #else
      interaction_environment().push(list(key, std::forward<Ts>(args)...));
      std::cerr << "; define\t; " << caar(interaction_environment()) << "\r\x1b[40C\x1b[K " << cadar(interaction_environment()) << std::endl;
      return interaction_environment();
    #endif
    }

    objective compile(const objective& exp,
                      const objective& scope = unit,
                      const objective& continuation = list(_stop_))
    {
      if (not exp)
      {
        return cons(_ldc_, unit, continuation);
      }
      else if (not exp.is<pair>())
      {
        TRACE("compile") << exp << " ; => ";

        if (exp.is<symbol>()) // is variable
        {
          if (auto location {locate(exp, scope)}; location) // there is local-defined variable
          {
            // load variable value (bound to lambda parameter) at runtime
            std::cerr << "is local variable => " << list(_ldl_, location) << std::endl;
            return cons(_ldl_, location, continuation);
          }
          else
          {
            // load variable value from global-environment at runtime
            std::cerr << "is global variable => " << list(_ldg_, exp) << std::endl;
            return cons(_ldg_, exp, continuation);
          }
        }
        else // is self-evaluation
        {
          std::cerr << "is self-evaluation => " << list(_ldc_, exp) << std::endl;
          return cons(_ldc_, exp, continuation);
        }
      }
      else // is (application . arguments)
      {
        if (const objective& buffer {assoc(car(exp), interaction_environment())};
            /* std::cerr << "." << std::flush, */ !buffer)
        {
          TRACE("compile") << "(" << car(exp) << " ; => is application of unit => ERROR" << std::endl;
          throw error {"unit is not applicable"};
        }
        else if ((/* std::cerr << "." << std::flush, */ buffer != unbound) &&
                 (/* std::cerr << "." << std::flush, */ buffer.is<special>()) &&
                 (/* std::cerr << "." << std::flush, */ not locate(car(exp), scope)))
        {
          TRACE("compile") << "(" << car(exp) << " ; => is application of ";
          std::cerr << buffer << std::endl;
          NEST_IN;
          auto result {std::invoke(buffer.as<special>(), cdr(exp), scope, continuation)};
          NEST_OUT;
          return result;
        }
        else if ((/* std::cerr << "." << std::flush, */ buffer != unbound) &&
                 (/* std::cerr << "." << std::flush, */ buffer.is<Enclosure>()) &&
                 (/* std::cerr << "." << std::flush, */ not locate(car(exp), scope)))
        {
          TRACE("compile") << "(" << car(exp) << " ; => is use of " << buffer << " => " << std::flush;

          auto& macro {unsafe_assoc(car(exp), interaction_environment()).template as<Enclosure&>()};
          // auto expanded {macro.expand(cdr(exp), interaction_environment())};
          auto expanded {macro.expand(cdr(exp))};
          TRACE("macroexpand") << expanded << std::endl;

          NEST_IN;
          auto result {compile(expanded, scope, continuation)};
          NEST_OUT;
          return result;
        }
        else // is (closure . arguments)
        {
          TRACE("compile") << "( ; => is any application " << std::endl;

          NEST_IN;
          auto result {operand(
                   cdr(exp),
                   scope,
                   compile(car(exp), scope, cons(_apply_, continuation))
                 )};
          NEST_OUT;
          return result;
        }
      }
    }

    decltype(auto) execute(const objective& exp)
    {
      c = exp;
      std::cerr << "; machine\t; " << c << std::endl;
      return execute();
    }

    objective execute()
    {
    dispatch:
      switch (c.top().as<instruction>().code)
      {
      case secd::LDL: // S E (LDL (i . j) . C) D => (value . S) E C D
        {
          // DEBUG(2);

          // Distance to target stack frame from current stack frame.
          int i {caadr(c).as<number>()};

          // Index of target value in the target stack frame.
          // If value is lower than 0, the target value is variadic parameter.
          int j {cdadr(c).as<number>()};

          // TODO Add LDV (load-variadic) instruction to remove this conditional.
          if (cursor region {car(std::next(e, i))}; j < 0)
          {
            s.push(std::next(region, -++j));
          }
          else
          {
            s.push(car(std::next(region, j)));
          }
        }
        c.pop(2);
        goto dispatch;

      case secd::LDC: // S E (LDC constant . C) D => (constant . S) E C D
        DEBUG(2);
        s.push(cadr(c));
        c.pop(2);
        goto dispatch;

      case secd::LDG: // S E (LDG symbol . C) D => (value . S) E C D
        DEBUG(2);
        if (auto value {assoc(cadr(c), interaction_environment())}; value != unbound)
        {
          s.push(value);
        }
        else
        {
          throw error {cadr(c), " is unbound"};
        }
        c.pop(2);
        goto dispatch;

      case secd::LDM: // S E (LDM code . C) => (enclosure . S) E C D
        DEBUG(2);
        s.push(make<Enclosure>(cadr(c), interaction_environment())); // レキシカル環境が必要ないのかはよく分からん
        c.pop(2);
        goto dispatch;

      case secd::LDF: // S E (LDF code . C) => (closure . S) E C D
        DEBUG(2);
        s.push(make<closure>(cadr(c), e));
        c.pop(2);
        goto dispatch;

      case secd::SELECT: // (boolean . S) E (SELECT then else . C) D => S E then/else (C. D)
        DEBUG(3);
        d.push(cdddr(c));
        c = car(s) != _false_ ? cadr(c) : caddr(c);
        s.pop(1);
        goto dispatch;

      case secd::JOIN: // S E (JOIN . x) (C . D) => S E C D
        DEBUG(1);
        c = car(d);
        d.pop(1);
        goto dispatch;

      case secd::CAR:
        DEBUG(1);
        car(s) = caar(s); // TODO check?
        c.pop(1);
        goto dispatch;

      case secd::CDR:
        DEBUG(1);
        car(s) = cdar(s); // TODO check?
        c.pop(1);
        goto dispatch;

      case secd::CONS:
        DEBUG(1);
        s = cons(cons(car(s), cadr(s)), cddr(s)); // s = car(s) | cadr(s) | cddr(s);
        c.pop(1);
        goto dispatch;

      case secd::DEFINE:
        DEBUG(2);
        define(cadr(c), car(s));
        car(s) = cadr(c); // return value of define (change to #<undefined>?)
        c.pop(2);
        goto dispatch;

      case secd::STOP: // (result . S) E (STOP . C) D
        DEBUG(1);
        c.pop(1);
        return s.pop(); // car(s);

      case secd::APPLY:
        DEBUG(1);

        if (auto applicable {car(s)}; not applicable)
        {
          throw error {"unit is not appliciable"};
        }
        else if (applicable.is<closure>()) // (closure args . S) E (APPLY . C) D
        {
          d.push(cddr(s), e, cdr(c));
          c = car(applicable);
          e = cons(cadr(s), cdr(applicable));
          s = unit;
        }
        else if (applicable.is<procedure>()) // (procedure args . S) E (APPLY . C) D => (result . S) E C D
        {
          s = std::invoke(applicable.as<procedure>(), cadr(s)) | cddr(s);
          c.pop(1);
        }
        else
        {
          throw error {applicable, "\x1b[31m", " is not applicable"};
        }
        goto dispatch;

      case secd::RETURN: // (value . S) E (RETURN . C) (S' E' C' . D) => (value . S') E' C' D
        DEBUG(1);
        s = cons(car(s), d.pop());
        e = d.pop();
        c = d.pop();
        goto dispatch;

      case secd::POP: // (var . S) E (POP . C) D => S E C D
        DEBUG(1);
        s.pop(1);
        c.pop(1);
        goto dispatch;

      case secd::SETG: // (value . S) E (SETG symbol . C) D => (value . S) E C D
        DEBUG(2);
        // TODO
        // (1) There is no need to make copy if right hand side is unique.
        // (2) There is no matter overwrite if left hand side is unique.
        // (3) Should set with weak reference if right hand side is newer.
        std::atomic_store(&unsafe_assoc(cadr(c), interaction_environment()), car(s).access().copy());
        c.pop(2);
        goto dispatch;

      case secd::SETL: // (var . S) E (SETG (i . j) . C) D => (var . S) E C D
        {
          DEBUG(2);

          // Distance to target stack frame from current stack frame.
          int i {caadr(c).as<number>()};

          // Index of target value in the target stack frame.
          // If value is lower than 0, the target value is variadic parameter.
          int j {cdadr(c).as<number>()};

          // TODO Add SETV (set-variadic) instruction to remove this conditional.
          auto& tmp {e};

          while (0 < i--)
          {
            tmp = cdr(tmp);
          }

          if (auto& region {car(tmp)}; j < 0)
          {
            // std::next(scope, -++j) <= car(s);
            auto& var {region};
            while (++j < -1) // ここ自信ない（一つ多いか少ないかも）
            {
              var = cdr(var);
            }
            std::atomic_store(&var, car(s));
          }
          else
          {
            // car(std::next(scope, j)) <= car(s);
            auto& var {region};
            while (0 < j--)
            {
              var = cdr(var);
            }
            std::atomic_store(&car(var), car(s));
          }
        }
        c.pop(2);
        goto dispatch;

      default:
        // XXX この式、実行されない（switchの方チェックの時点で例外で出て行く）
        throw error {car(c), "\x1b[31m is not virtual machine instruction"};
      }

      // XXX この式、実行されない（そもそもたどり着かない）
      throw error {"unterminated execution"};
    }

    // De Bruijn Index
    objective locate(const objective& variable,
                     const objective& lexical_environment)
    {
      auto i {0};

      // for (cursor region {lexical_environment}; region; ++region)
      for (const auto& region : lexical_environment)
      {
        auto j {0};

        for (cursor y {region}; y; ++y)
        {
          if (y.is<pair>() && car(y) == variable)
          {
            return cons(make<number>(i), make<number>(j));
          }
          else if (!y.is<pair>() && y == variable)
          {
            return cons(make<number>(i), make<number>(-++j));
          }

          ++j;
        }

        ++i;
      }

      return unit;
    }

  protected:
    /* 7.1.3
     *
     * <sequence> = <command>* <expression>
     *
     * <command> = <expression>
     *
     */
    objective sequence(const objective& expression,
                       const objective& region,
                       const objective& continuation)
    {
      return compile(
               car(expression),
               region,
               cdr(expression) ? cons(
                                   _pop_,
                                   sequence(cdr(expression), region, continuation)
                                 )
                               : continuation
             );
    }

    /*  7.1.3
     *
     * <body> = <definition>* <sequence>
     *
     */
    objective body(const objective& expression,
                   const objective& region,
                   const objective& continuation) try
    {
      return compile(
               car(expression),
               region,
               cdr(expression) ? cons(
                                   _pop_,
                                   sequence(cdr(expression), region, continuation)
                                 )
                               : continuation
             );
    }
    catch (const error&) // internal define backtrack
    {
      std::cerr << cadar(expression) << " ; => is local-variable" << std::endl;
      throw;

      // (lambda ()
      //   (define <cadar> . <cddar>)
      //   <cdr>
      // )
      //
      // (lambda ()
      //   ((lambda (<cadar>)
      //      (set! (0 . 0) . <cddar>)
      //      ...)
      //    #<undefined>)
      // )
    }

    /* 7.1.3
     *
     * <operand> = <expression>
     *
     */
    objective operand(const objective& expression,
                      const objective& region,
                      const objective& continuation)
    {
      if (expression && expression.is<pair>())
      {
        return operand(
                 cdr(expression),
                 region,
                 compile(car(expression), region, cons(_cons_, continuation))
               );
      }
      else
      {
        return compile(expression, region, continuation);
      }
    }

    objective let(const objective& expression,
                  const objective& lexical_environment,
                  const objective& continuation)
    {
      auto binding_specs {car(expression)};

      return operand(
               map([](auto&& e) { return cadr(e); }, binding_specs), // <arguments>
               lexical_environment,
               cons(
                 _ldf_,
                 body(
                   cdr(expression), // <body>
                   cons(
                     map([](auto&& e) { return car(e); }, binding_specs), // <formals>
                     lexical_environment
                   ),
                   list(_return_)
                 ),
                 _apply_, continuation
               )
             );
    }
  };
} // namespace meevax::system

#endif // INCLUDED_MEEVAX_SYSTEM_MACHINE_HPP

