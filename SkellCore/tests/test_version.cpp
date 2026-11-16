#include <SkellCore/Version.h>
#include <cassert>
#include <cstdint>

int main() {
  using skellcore::version::parse;
  using skellcore::version::newer;
  assert(parse("v1.2.3").valid);
  assert(parse("V0.10.42+build.1").valid);
  assert(parse("2.4.0-rc.12+release").valid);
  assert(!parse("1.2").valid);
  assert(!parse("1.2.3garbage").valid);
  assert(!parse("v1.2.3-").valid);
  assert(!parse("1.2.3+bad..meta").valid);
  assert(!parse("1.2.4294967296").valid);
  assert(!parse("2.0.0\n").valid);
  assert(newer("v1.9.9", "v1.10.0"));
  assert(newer("1.0.0", "2.0.0"));
  assert(!newer("1.4.2", "1.4.2"));
  assert(!newer("2.0.0", "1.99.99"));
  assert(!newer("1.4.2", "not a release"));
  assert(!newer("1.4.2", "1.5.0-"));
  assert(newer("1.2.0-rc.2", "1.2.0"));
  assert(newer("1.2.0-rc.2", "1.2.0-rc.10"));
  assert(!newer("1.2.0", "1.2.0-rc.10"));
  assert(newer("1.2.0+local", "1.2.1+release"));
  assert(!newer("1.2.0+local", "1.2.0+release"));
  return 0;
}
