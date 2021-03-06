#include <meevax/kernel/basis.hpp>

/* ---- Embedded Source Codes --------------------------------------------------
 *
 *  basis/hoge.ss
 *
 *  NOTE: readelf -a hoge.ss.o
 *
 * -------------------------------------------------------------------------- */

#define DEFINE_BINARY(FILENAME)                                                \
                                                                               \
extern "C" const char _binary_##FILENAME##_ss_start[];                         \
extern "C" const char _binary_##FILENAME##_ss_end[];                           \
extern "C" const char _binary_##FILENAME##_ss_size[];                          \
                                                                               \
namespace meevax                                                               \
{                                                                              \
inline namespace kernel                                                        \
{                                                                              \
  string_view const FILENAME {                                                 \
    _binary_##FILENAME##_ss_start,                                             \
    static_cast<std::size_t>(                                                  \
      _binary_##FILENAME##_ss_end - _binary_##FILENAME##_ss_start)             \
  };                                                                           \
}                                                                              \
} static_assert(true)

DEFINE_BINARY(overture);
DEFINE_BINARY(r7rs);
DEFINE_BINARY(srfi_1);
DEFINE_BINARY(srfi_8);
DEFINE_BINARY(srfi_23);
DEFINE_BINARY(srfi_34);
DEFINE_BINARY(srfi_39);
DEFINE_BINARY(srfi_45);
DEFINE_BINARY(srfi_78);
