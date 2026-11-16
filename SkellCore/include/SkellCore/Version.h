#pragma once
// Portable, allocation-free SemVer comparison for update offers. Never used on audio threads.
#include <cstdint>
#include <limits>
#include <string_view>

namespace skellcore::version {
struct Number {
  std::uint32_t major = 0, minor = 0, patch = 0;
  std::string_view prerelease{};
  bool valid = false;
};

inline bool readNumber(std::string_view text, std::size_t& at, std::uint32_t& value) noexcept {
  const auto start = at;
  value = 0;
  while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
    const auto digit = static_cast<std::uint32_t>(text[at++] - '0');
    if (value > (std::numeric_limits<std::uint32_t>::max() - digit) / 10u)
      return false;
    value = value * 10u + digit;
  }
  return at != start;
}

inline bool validIdentifiers(std::string_view text) noexcept {
  if (text.empty() || text.front() == '.' || text.back() == '.') return false;
  bool dot = false;
  for (const auto c : text) {
    if (c == '.') { if (dot) return false; dot = true; continue; }
    dot = false;
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') || c == '-')) return false;
  }
  return true;
}

inline Number parse(std::string_view text) noexcept {
  Number out;
  if (!text.empty() && (text.front() == 'v' || text.front() == 'V'))
    text.remove_prefix(1);
  std::size_t at = 0;
  if (!readNumber(text, at, out.major) || at == text.size() || text[at++] != '.' ||
      !readNumber(text, at, out.minor)) return out;
  // Two-component product versions (v2.5) are treated as v2.5.0.
  if (at < text.size() && text[at] == '.') {
    ++at;
    if (!readNumber(text, at, out.patch)) return out;
  }
  if (at < text.size() && text[at] == '-') {
    const auto start = ++at;
    while (at < text.size() && text[at] != '+') ++at;
    out.prerelease = text.substr(start, at - start);
    if (!validIdentifiers(out.prerelease)) return out;
  }
  if (at < text.size() && text[at] == '+') {
    const auto meta = text.substr(at + 1);
    if (!validIdentifiers(meta)) return out;
    at = text.size();
  }
  out.valid = at == text.size();
  return out;
}

inline int compareIdentifiers(std::string_view a, std::string_view b) noexcept {
  while (true) {
    const auto aDot = a.find('.'), bDot = b.find('.');
    const auto x = a.substr(0, aDot), y = b.substr(0, bDot);
    const auto numeric = [](std::string_view s) {
      for (const auto c : s) if (c < '0' || c > '9') return false;
      return !s.empty();
    };
    const bool xn = numeric(x), yn = numeric(y);
    if (xn && yn) {
      // Compare number tokens without integer overflow.
      const auto strip = [](std::string_view s) {
        const auto n = s.find_first_not_of('0');
        return n == std::string_view::npos ? s.substr(s.size() - 1) : s.substr(n);
      };
      const auto xx = strip(x), yy = strip(y);
      if (xx.size() != yy.size()) return xx.size() < yy.size() ? -1 : 1;
      if (xx != yy) return xx < yy ? -1 : 1;
    } else if (xn != yn) {
      return xn ? -1 : 1;
    } else if (x != y) {
      return x < y ? -1 : 1;
    }
    const bool aEnd = aDot == std::string_view::npos, bEnd = bDot == std::string_view::npos;
    if (aEnd || bEnd) return aEnd == bEnd ? 0 : aEnd ? -1 : 1;
    a.remove_prefix(aDot + 1);
    b.remove_prefix(bDot + 1);
  }
}

inline bool newer(std::string_view current, std::string_view remote) noexcept {
  const auto a = parse(current), b = parse(remote);
  if (!a.valid || !b.valid) return false; // malformed update metadata is never trusted
  if (a.major != b.major) return b.major > a.major;
  if (a.minor != b.minor) return b.minor > a.minor;
  if (a.patch != b.patch) return b.patch > a.patch;
  if (a.prerelease.empty()) return false;
  if (b.prerelease.empty()) return true;
  return compareIdentifiers(a.prerelease, b.prerelease) < 0;
}
} // namespace skellcore::version
