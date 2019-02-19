#ifndef INCLUDED_MEEVAX_CHARACTER_UNICODE_HPP
#define INCLUDED_MEEVAX_CHARACTER_UNICODE_HPP

#include <string>

#include <meevax/concepts/is_character.hpp>

namespace meevax::character
{
  template <auto N>
  class unicode;

  template <>
  class unicode<8>
    : public std::string
  {};

  DEFINE_CONCEPT_IS_CHARACTER_SPECIALIZATION(unicode<8>)
} // namespace meevax::character

#endif // INCLUDED_MEEVAX_CHARACTER_UNICODE_HPP

