#include <SkellCore/ReleasePolicy.h>
#include <cstdio>
#include <string>
#include <limits>

#define CHECK(expr) do { \
  if (!(expr)) { \
    std::fprintf(stderr, "Release policy FAILED line %d: %s\n", __LINE__, #expr); \
    return 1; \
  } \
} while (false)

int main() {
  using skellcore::updates::Result;
  using skellcore::updates::classify;
  using skellcore::updates::validReleaseTag;
  using skellcore::updates::validRepositoryPart;
  CHECK(validRepositoryPart("skellyruno", true));
  CHECK(validRepositoryPart("skell-preset-vst", false));
  CHECK(validRepositoryPart(".github", false));
  CHECK(!validRepositoryPart("", true));
  CHECK(!validRepositoryPart("-owner", true));
  CHECK(!validRepositoryPart("owner-", true));
  CHECK(!validRepositoryPart("owner/repo", true));
  CHECK(!validRepositoryPart("repo space", false));
  CHECK(!validRepositoryPart("bad/repo", false));
  CHECK(!validRepositoryPart("..", false));
  CHECK(!validRepositoryPart(std::string(40, 'a'), true));
  CHECK(!validRepositoryPart(std::string(101, 'a'), false));
  CHECK(validReleaseTag("v1.4"));
  CHECK(validReleaseTag("1.4.0+build.7"));
  CHECK(!validReleaseTag("v1.4.0/../../evil"));
  CHECK(!validReleaseTag("v1.4.0?download"));
  CHECK(!validReleaseTag("v1.04.0"));
  CHECK(!validReleaseTag(std::string(80, 'a')));
  CHECK(classify("1.0.0", "skellyruno", "repo", 0) == Result::Offline);
  CHECK(classify("1.0.0", "skellyruno", "repo", -1) == Result::Offline);
  CHECK(classify("1.0.0", "skellyruno", "repo", 404) == Result::NoPublishedRelease);
  CHECK(classify("1.0.0", "skellyruno", "repo", 401) == Result::AuthenticationRequired);
  CHECK(classify("1.0.0", "skellyruno", "repo", 403) == Result::RateLimited);
  CHECK(classify("1.0.0", "skellyruno", "repo", 429) == Result::RateLimited);
  CHECK(classify("1.0.0", "skellyruno", "repo", 502) == Result::ServiceUnavailable);
  CHECK(classify("1.0.0", "skellyruno", "repo", 200, "") == Result::UnsupportedRelease);
  CHECK(classify("1.0.0", "skellyruno", "repo", 200, "v1.0.1", true) == Result::UnsupportedRelease);
  CHECK(classify("1.0.0", "skellyruno", "repo", 200, "v1.0.1", false, true) == Result::UnsupportedRelease);
  CHECK(classify("1.0.0", "skellyruno", "repo", 200, "v1.0.1") == Result::UpdateAvailable);
  CHECK(classify("v1.9.9", "skellyruno", "repo", 200, "v1.10.0") == Result::UpdateAvailable);
  CHECK(classify("2.5", "skellyruno", "repo", 200, "2.5.0") == Result::Current);
  CHECK(classify("2.5", "skellyruno", "repo", 200, "2.4.9") == Result::Current);
  CHECK(classify("1.4.0", "skellyruno", "repo", 200, "1.4.0-rc.2") == Result::Current);
  CHECK(classify("1.4.0-rc.2", "skellyruno", "repo", 200, "1.4.0") == Result::UpdateAvailable);
  CHECK(classify("1.4.0", "skellyruno", "repo", 200, "v1.5.0/evil") == Result::UnsupportedRelease);
  CHECK(classify("1.4.0", "skellyruno", "", 200, "1.5.0") == Result::InvalidConfiguration);
  CHECK(classify("not-version", "skellyruno", "repo", 200, "1.5.0") == Result::InvalidConfiguration);
  CHECK(classify("1.4.0", "skellyruno", "repo/evil", 200, "1.5.0") == Result::InvalidConfiguration);
  std::puts("Release policy: 37+ explicit offline, security, stability and version checks passed");
  return 0;
}
