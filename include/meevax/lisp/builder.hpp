#ifndef INCLUDED_MEEVAX_LISP_BUILDER_HPP
#define INCLUDED_MEEVAX_LISP_BUILDER_HPP

#include <iterator>
#include <list>
#include <string>

#include <meevax/lisp/cell.hpp>
#include <meevax/lisp/list.hpp>
#include <meevax/lisp/table.hpp>
#include <meevax/utility/fold.hpp>

namespace meevax::lisp
{
  class builder
    : public std::list<builder>
  {
    std::string value_;

  public:
    builder(const std::string& value)
      : value_ {value}
    {}

    template <typename InputIterator>
    explicit builder(InputIterator&& begin, InputIterator&& end)
    {
      if (std::distance(begin, end) != 0)
      {
        if (*begin != "(")
        {
          if (*begin == "'")
          {
            emplace_back("quote");
            emplace_back(++begin, end);
          }
          else value_ = *begin;
        }
        else while (++begin != end && *begin != ")")
        {
          emplace_back(begin, end);
        }
      }
    }

    decltype(auto) operator()() const
    {
      return build();
    }

  protected:
    cursor build() const
    {
      if (std::empty(*this))
      {
        return std::empty(value_) ? lookup("nil", symbols) : intern(value_, symbols);
      }
      else
      {
        using namespace utility;
        return fold_right(std::begin(*this), std::end(*this), lookup("nil", symbols), [](auto& head, auto& tail)
        {
          return head() | tail;
        });
      }
    }
  };
} // namespace meevax::lisp

#endif // INCLUDED_MEEVAX_LISP_BUILDER_HPP

