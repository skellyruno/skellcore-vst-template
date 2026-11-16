#pragma once
// Portable, deterministic release-check policy. No JUCE, networking or audio-thread work.
// UI/network code must treat the response body as untrusted and call this function.
#include <SkellCore/Version.h>
#include <string_view>

namespace skellcore::updates {

enum class Result {
  InvalidConfiguration,
  Offline,
  NoPublishedRelease,
  AuthenticationRequired,
  RateLimited,
  ServiceUnavailable,
  UnsupportedRelease,
  Current,
  UpdateAvailable
};

inline bool validRepositoryPart(std::string_view text, bool owner) noexcept {
  if (text.empty() || text.size() > (owner ? 39u : 100u) ||
      text == "." || text == "..") return false;
  if (owner && (text.front() == '-' || text.back() == '-')) return false;
  for (const char c : text) {
    const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = c >= '0' && c <= '9';
    const bool extra = c == '-' || (!owner && (c == '.' || c == '_'));
    if (!(alpha || digit || extra)) return false;
  }
  return true;
}

inline bool validReleaseTag(std::string_view tag) noexcept {
  if (tag.empty() || tag.size() > 79 || !version::parse(tag).valid)
    return false;
  for (const char c : tag) {
    const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = c >= '0' && c <= '9';
    if (!alpha && !digit && c != '.' && c != '-' && c != '+' && c != '_')
      return false;
  }
  return true;
}

// HTTP and JSON parsing belong to the off-audio-thread caller. This policy is
// pure so all states, including network failures, can be tested without GitHub.
inline Result classify(std::string_view installed, std::string_view owner,
                       std::string_view repository, int httpStatus,
                       std::string_view releaseTag = {},
                       bool draft = false, bool prerelease = false) noexcept {
  if (!version::parse(installed).valid ||
      !validRepositoryPart(owner, true) ||
      !validRepositoryPart(repository, false))
    return Result::InvalidConfiguration;
  if (httpStatus <= 0) return Result::Offline;
  if (httpStatus == 401) return Result::AuthenticationRequired;
  if (httpStatus == 403 || httpStatus == 429) return Result::RateLimited;
  if (httpStatus == 404) return Result::NoPublishedRelease;
  if (httpStatus != 200) return Result::ServiceUnavailable;
  if (draft || prerelease || !validReleaseTag(releaseTag))
    return Result::UnsupportedRelease;
  return version::newer(installed, releaseTag)
    ? Result::UpdateAvailable : Result::Current;
}
} // namespace skellcore::updates
