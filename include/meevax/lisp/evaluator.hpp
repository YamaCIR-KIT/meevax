#ifndef INCLUDED_MEEVAX_LISP_EVALUATOR_HPP
#define INCLUDED_MEEVAX_LISP_EVALUATOR_HPP

#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>

#include <meevax/functional/combinator.hpp>
#include <meevax/lisp/cell.hpp>
#include <meevax/lisp/exception.hpp>
#include <meevax/lisp/writer.hpp> // to_string

namespace meevax::lisp
{
  class evaluator
  {
    cursor env_;

    std::unordered_map<
      std::shared_ptr<cell>,
      std::function<cursor (cursor, cursor)>
    > procedure;

    struct closure
    {
      cursor exp, env;
    };

  public:
    evaluator()
      : env_ {symbols("nil")}
    {
      using namespace meevax::functional;

      define("quote", [](auto e, auto)
      {
        return *++e;
      });

      define("atom", [&](auto e, auto a)
      {
        return atom(evaluate(*cdr(e), a)) ? symbols.intern("true") : symbols("nil");
      });

      define("eq", [&](auto e, auto a)
      {
        return evaluate(*++e, a) == evaluate(*++e, a) ? symbols.intern("true") : symbols("nil");
      });

      define("if", [&](auto e, auto a)
      {
        return evaluate(*++e, a) ? evaluate(cadr(e), a) : evaluate(caddr(e), a);
      });

      define("cond", [&](auto exp, auto env)
      {
        return z([&](auto&& proc, auto&& exp, auto&& env) -> cursor
        {
          return evaluate(**exp, env) ? evaluate(cadar(exp), env) : proc(proc, ++exp, env);
        })(++exp, env);
      });

      define("car", [&](auto e, auto a)
      {
        return *evaluate(*++e, a);
      });

      define("cdr", [&](auto e, auto a)
      {
        return ++evaluate(*++e, a);
      });

      define("cons", [&](auto e, auto a)
      {
        return evaluate(cadr(e), a) | evaluate(caddr(e), a);
      });

      define("lambda", [&](auto exp, auto env)
      {
        using binder = utility::binder<closure, cell>;
        return std::make_shared<binder>(exp, env);
               //   std::forward<decltype(exp)>(exp),
               //   std::forward<decltype(env)>(env)
               // );
      } );

      define("define", [&](auto value, auto)
      {
        return lookup(cadr(value), env_ = list(cadr(value), caddr(value)) | env_);
      });

      define("list", [&](auto e, auto a)
      {
        return z([&](auto proc, auto e, auto a) -> cursor
        {
          return evaluate(*e, a) | (cdr(e) ? proc(proc, cdr(e), a) : symbols("nil"));
        })(++e, a);
      });

      define("exit", [&](auto, auto)
        -> cursor
      {
        std::exit(boost::exit_success);
      });
    }

    template <typename T>
    decltype(auto) operator()(T&& e)
    {
      return evaluate(std::forward<T>(e), env_);
    }

    template <typename S, typename F>
    void define(S&& s, F&& functor)
    {
      procedure.emplace(symbols.intern(s), functor);
    }

  protected:
    cursor evaluate_(cursor e, cursor a)
    {
      if (atom(e))
      {
        // TODO self evaluating?

        // TODO variable?
        return lookup(e, a);
        //     ~~~~~~~~~~~~
        //     ^ lookup variable value
      }
      else if (atom(*e))
      {
        return invoke(e, a);
      }
      // else if (**e == symbols.intern("recursive"))
      // {
      //   return evaluate(caddar(e) | cdr(e), list(cadar(e), *e) | a);
      // }
      else if (**e == symbols.intern("lambda"))
      {
        // ((lambda (params...) body) args...)

        return evaluate(caddar(e), append(zip(cadar(e), evlis(cdr(e), a)), a));
        //          ~~~~~~~~~             ~~~~~~~~        ~~~~~~
        //          ^ body                ^ params        ^ args
      }
      else if (**e == symbols.intern("macro"))
      {
        // ((macro (params...) (body...)) args...)

        const auto expanded {evaluate(caddar(e), append(zip(cadar(e), cdr(e)), a))};
        //                            ~~~~~~~~~             ~~~~~~~~  ~~~~~~
        //                            ^ body                ^ params  ^ args

        std::cerr << "-> " << expanded << std::endl;

        return evaluate(expanded, a);
      }
      else throw generate_exception(
        "unexpected evaluation dispatch failure for expression " + to_string(e)
      );
    }

    cursor evaluate(cursor exp, cursor env)
    {
      if (atom(exp))
      {
        return lookup(exp, env);
      }

      if (auto iter {procedure.find(car(exp))}; iter != std::end(procedure))
      {
        return (iter->second)(exp, env);
      }

      if (auto callee {evaluate(car(exp), env)}; callee)
      {
        if (callee.access().type() == typeid(closure))
        {
          const auto closure_ {(callee).access().as<closure>()};
          return evaluate(
                   caddr(closure_.exp),
                   append(
                     zip(cadr(closure_.exp), evlis(cdr(exp), env)),
                     closure_.env
                   )
                 );
        }
        else
        {
          return evaluate(callee | cdr(exp), env);
        }
      }

      throw generate_exception(
        "unexpected evaluation dispatch failure for expression " + to_string(exp)
      );
    }

  private:
    cursor invoke(cursor e, cursor a)
    {
      if (const auto iter {procedure.find(*e)}; iter != std::end(procedure))
      //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //  ^ primitive procedure?
      {
        return iter->second(e, a);
        //     ~~~~~~~~~~~~~~~~~~
        //     ^ apply primive procedure
      }
      else if (auto callee {evaluate(*e, a)}; callee)
      //       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //       ^ compound procedure?
      {
        return evaluate(callee | cdr(e), a);
      }
      else throw generate_exception(
        "using inapplicable symbol " + to_string(*e) + " as procedure"
      );
    }

    template <typename... Ts>
    cursor list(Ts&&... args)
    {
      return (args | ... | symbols("nil"));
    };

    cursor append(cursor x, cursor y)
    {
      return !x ? y : *x | append(cdr(x), y);
    }

    cursor zip(cursor x, cursor y)
    {
      if (!x && !y)
      {
        return symbols("nil");
      }
      else if (!atom(x) && !atom(y))
      {
        return list(*x, *y) | zip(cdr(x), cdr(y));
      }
      else
      {
        return symbols("nil");
      }
    }

    cursor lookup(cursor var, cursor env)
    {
      return !var or !env ? symbols("nil") : var == **env ? cadar(env) : lookup(var, cdr(env));
    }

    cursor evlis(cursor m, cursor a)
    {
      return !m ? symbols("nil") : evaluate(*m, a) | evlis(cdr(m), a);
    }
  } static eval {};
} // namespace meevax::lisp

#endif // INCLUDED_MEEVAX_LISP_EVALUATOR_HPP

