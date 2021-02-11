#ifndef INCLUDED_MEEVAX_KERNEL_READER_HPP
#define INCLUDED_MEEVAX_KERNEL_READER_HPP

#include <boost/lexical_cast.hpp>
#include <limits> // std::numeric_limits<std::streamsize>
#include <sstream>
#include <stack>

// #include <boost/iostreams/device/null.hpp>
// #include <boost/iostreams/stream_buffer.hpp>

#include <meevax/iostream/ignore.hpp>
#include <meevax/kernel/boolean.hpp>
#include <meevax/kernel/ghost.hpp>
#include <meevax/kernel/list.hpp>
#include <meevax/kernel/miscellaneous.hpp>
#include <meevax/kernel/numeric_io.hpp>
#include <meevax/kernel/parser.hpp> // DEPRECATED
#include <meevax/kernel/path.hpp>
#include <meevax/kernel/port.hpp>
#include <meevax/kernel/string.hpp>
#include <meevax/kernel/symbol.hpp>
#include <meevax/kernel/vector.hpp>
#include <meevax/string/header.hpp>

namespace meevax
{
inline namespace kernel
{
  auto read_token(input_port & port) -> std::string;

  /* ---- R7RS 6.13.2. Input ---------------------------------------------------
   *
   *  (read-char)                                                     procedure
   *  (read-char port)                                                procedure
   *
   *  Returns the next character available from the textual input port,
   *  updating the port to point to the following character. If no more
   *  characters are available, an end-of-file object is returned.
   *
   * ------------------------------------------------------------------------ */
  let read_char(input_port &);

  /* ---- R7RS 6.13.2. Input ---------------------------------------------------
   *
   *  (read-string k)                                                 procedure
   *  (read-string k port)                                            procedure
   *
   *  Reads the next k characters, or as many as are available before the end
   *  of file, from the textual input port into a newly allocated string in
   *  left-to-right order and returns the string. If no characters are
   *  available before the end of file, an end-of-file object is returned.
   *
   * ------------------------------------------------------------------------ */
  let read_string(input_port &);

  let make_string(std::string const&);

  /* ---- Reader ---------------------------------------------------------------
   *
   *
   * ------------------------------------------------------------------------ */
  template <typename SK>
  class reader
  {
    friend SK;

    explicit reader()
    {}

    IMPORT(SK, evaluate,            NIL);
    IMPORT(SK, intern,              NIL);
    IMPORT(SK, standard_debug_port, NIL);
    IMPORT(SK, write_to,            NIL);

    using seeker = std::istream_iterator<input_port::char_type>;

    enum class   proper_list_tag {};
    enum class improper_list_tag {};

  public:
    /* ---- Read ---------------------------------------------------------------
     *
     *
     * ---------------------------------------------------------------------- */
    let const read(input_port & port)
    {
      std::string token {};

      for (seeker head = port; head != seeker(); ++head)
      {
        switch (*head)
        {
        case ';':
          port.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          break;

        case ' ': case '\f': case '\n': case '\r': case '\t': case '\v':
          break;

        case '(':
          try
          {
            let const kar = read(port);
            port.putback('(');
            return cons(kar, read(port));
          }
          catch (read_error<proper_list_tag> const&)
          {
            return unit;
          }
          catch (read_error<improper_list_tag> const&)
          {
            let kdr = read(port);
            port.ignore(std::numeric_limits<std::streamsize>::max(), ')'); // XXX DIRTY HACK
            return kdr;
          }

        case ')':
          throw read_error<proper_list_tag>("unexpected close parentheses");

        case '#':
          return discriminate(port);

        case '"':
          return read_string(port);

        case '\'':
          return list(intern("quote"), read(port));

        case '`':
          return list(intern("quasiquote"), read(port));

        case ',':
          switch (port.peek())
          {
          case '@':
            port.ignore(1);
            return list(intern("unquote-splicing"), read(port));

          default:
            return list(intern("unquote"), read(port));
          }

        default:
          token.push_back(*head);

          if (auto const c = port.peek(); is_end_of_token(c))
          {
            if (token == ".")
            {
              throw read_error<improper_list_tag>("dot-notation");
            }
            else try
            {
              return to_number(token, 10);
            }
            catch (...)
            {
              return intern(token);
            }
          }
        }
      }

      return eof_object;
    }

    decltype(auto) read(object const& x)
    {
      if (x.is_polymorphically<input_port>())
      {
        return read(x.as<input_port>());
      }
      else
      {
        throw read_error<void>(__FILE__, ":", __LINE__);
      }
    }

    let const read()
    {
      let const result = read(standard_input_port());

      write_to(standard_debug_port(), "\n", header(__func__), result, "\n");

      return result;
    }

    decltype(auto) read(std::string const& s)
    {
      std::stringstream ss { s };
      return read(ss);
    }

    let standard_input_port() const noexcept
    {
      let static port = make<standard_input>();
      return port;
    }

    auto ready() // TODO RENAME TO 'char-ready'
    {
      return not standard_input_port().template is<null>() and standard_input_port().template as<input_port>();
    }

  private:
    let discriminate(input_port & is)
    {
      switch (auto const discriminator = is.get())
      {
      case '!':
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return read(is);

      case ',': // from SRFI-10
        return evaluate(read(is));

      case ';': // from SRFI-62
        return read(is), read(is);

      case 'b': // (string->number (read) 2)
        return to_number(is.peek() == '#' ? boost::lexical_cast<std::string>(read(is)) : read_token(is), 2);

      case 'c': // from Common Lisp
        if (let const xs = read(is); not xs.is<pair>())
        {
          return make<complex>(make<exact_integer>(0), make<exact_integer>(0));
        }
        else if (not cdr(xs).is<pair>())
        {
          return make<complex>(car(xs), make<exact_integer>(0));
        }
        else
        {
          return make<complex>(car(xs), cadr(xs));
        }

      case 'd':
        return to_number(is.peek() == '#' ? boost::lexical_cast<std::string>(read(is)) : read_token(is), 10);

      case 'e':
        return exact(read(is)); // NOTE: Same as #,(exact (read))

      case 'f':
        ignore(is, [](auto&& x) { return not is_end_of_token(x); });
        return f;

      case 'i':
        return inexact(read(is)); // NOTE: Same as #,(inexact (read))

      case 'o':
        return to_number(is.peek() == '#' ? boost::lexical_cast<std::string>(read(is)) : read_token(is), 8);

      case 'p':
        assert(is.get() == '"');
        is.ignore(1);
        return make<path>(read_string(is).template as<string>());

      case 't':
        ignore(is, [](auto&& x) { return not is_end_of_token(x); });
        return t;

      case 'x':
        return to_number(is.peek() == '#' ? boost::lexical_cast<std::string>(read(is)) : read_token(is), 16);

      case '(':
        is.putback(discriminator);
        return make<vector>(for_each_in, read(is));

      case '\\':
        return read_char(is);

      default:
        throw read_error<void>("unknown <discriminator>: #", discriminator);
      }
    }
  };
} // namespace kernel
} // namespace meevax

#endif // INCLUDED_MEEVAX_KERNEL_READER_HPP
