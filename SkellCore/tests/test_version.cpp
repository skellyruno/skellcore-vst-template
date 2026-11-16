#include <SkellCore/Version.h>
#include <cstdio>
// Never compile away our test assertions in Release/NDEBUG builds.
#define CHECK(expr) do { \
  if (!(expr)) { \
    std::fprintf(stderr, "Version FAILED at line %d: %s\\n", __LINE__, #expr); \
    return 1; \
  } \
} while (false)
#include <cstdint>

int main() {
  using skellcore::version::parse;
  using skellcore::version::newer;
  CHECK(parse("v1.2.3").valid);
  CHECK(parse("V0.10.42+build.1").valid);
  CHECK(parse("2.4.0-rc.12+release").valid);
  CHECK(parse("1.2").valid);
  CHECK(!parse("1").valid);
  CHECK(newer("v2.4", "v2.5"));
  CHECK(parse("0.0").valid);
  CHECK(!parse("01.2.3").valid);
  CHECK(!parse("1.02.3").valid);
  CHECK(!parse("1.2.03").valid);
  CHECK(!parse("1.2.3-rc.01").valid);
  CHECK(!parse("1.2.3-001").valid);
  CHECK(parse("1.2.3-0").valid);
  CHECK(parse("1.2.3-rc.0").valid);
  CHECK(parse("1.2.3-rc.01a").valid);
  CHECK(parse("1.2.3+build.0001").valid);
  CHECK(!newer("v1.2.3", "v01.4.0"));
  CHECK(newer("1.0.0-rc.9", "1.0.0-rc.10"));
  CHECK(newer("1.0.0-rc.9999999999999999999999999999", "1.0.0-rc.10000000000000000000000000000"));
  CHECK(!newer("2.5.0", "v2.5"));
  CHECK(!parse("1.2.3garbage").valid);
  CHECK(!parse("v1.2.3-").valid);
  CHECK(!parse("1.2.3+bad..meta").valid);
  CHECK(!parse("1.2.4294967296").valid);
  CHECK(!parse("2.0.0\n").valid);
  CHECK(newer("v1.9.9", "v1.10.0"));
  CHECK(newer("1.0.0", "2.0.0"));
  CHECK(!newer("1.4.2", "1.4.2"));
  CHECK(!newer("2.0.0", "1.99.99"));
  CHECK(!newer("1.4.2", "not a release"));
  CHECK(!newer("1.4.2", "1.5.0-"));
  CHECK(newer("1.2.0-rc.2", "1.2.0"));
  CHECK(newer("1.2.0-rc.2", "1.2.0-rc.10"));
  CHECK(!newer("1.2.0", "1.2.0-rc.10"));
  CHECK(newer("1.2.0+local", "1.2.1+release"));
  CHECK(!newer("1.2.0+local", "1.2.0+release"));
  return 0;
}
