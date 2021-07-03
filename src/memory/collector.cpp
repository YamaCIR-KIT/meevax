#include <meevax/memory/collector.hpp>

namespace meevax
{
inline namespace memory
{
  static std::size_t count = 0;

  collector::collector()
  {
    if (not count++)
    {
      roots = {};

      regions = {};

      collecting = false;

      newly_allocated = 0;

      threshold = std::numeric_limits<std::size_t>::max();
      // threshold = 1024 * 1024; // = 1 MiB
    }
  }

  collector::~collector()
  {
    if (not --count)
    {
      /* ---- NOTE -------------------------------------------------------------
       *
       *  We're using collect instead of clear to check that all objects can be
       *  collected. If speed is a priority, clear should be used here.
       *
       * -------------------------------------------------------------------- */

      collect();
      collect(); // XXX: vector elements

      assert(std::size(roots) == 0);
      assert(std::size(regions) == 0);
    }
  }
} // namespace memory
} // namespace meevax

auto operator new(std::size_t const size, meevax::collector & gc) -> meevax::pointer<void>
{
  auto const lock = gc.lock();

  if (gc.overflow(size))
  {
    gc.collect();
  }

  auto p = ::operator new(size);

  gc.insert(p, size);

  return p;
}

void operator delete(meevax::pointer<void> const p, meevax::collector & gc) noexcept
{
  auto const lock = gc.lock();

  try
  {
    if (auto const iter = gc.find(p); *iter)
    {
      gc.erase(iter);
    }
  }
  catch (...)
  {}

  ::operator delete(p);
}
