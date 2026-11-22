// Optimized regression tests must not compile assert() expressions away.
#ifdef NDEBUG
#error "NDEBUG disabled SkellCore regression assertions; check CMake test flags."
#endif
#include <cassert>
int main() {
  bool executed = false;
  assert((executed = true));
  return executed ? 0 : 1;
}
