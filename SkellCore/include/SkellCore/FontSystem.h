#pragma once
// Shared opt-in font helpers for JUCE 8 editor code. Never call on the audio thread.
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>

namespace skellcore::ui {
enum class FontRole { Text, Heading, Symbol, Emoji };

class FontSystem final {
public:
  // A requested installed or plugin-registered typeface is preserved. If the
  // family is unavailable, JUCE chooses a system fallback rather than failing.
  // Per-glyph fallback stays enabled: no single font contains every Unicode
  // codepoint, and OS font availability controls colour emoji rendering.
  static juce::Font make(float height, FontRole role = FontRole::Text,
                         const juce::String& requestedFamily = {}, bool bold = false) {
    const auto safeHeight = std::isfinite(height) ? juce::jlimit(8.0f, 120.0f, height) : 14.0f;
    const auto family = requestedFamily.isNotEmpty() ? requestedFamily
        : chooseFamily(role);
    auto result = juce::Font(juce::FontOptions(family, safeHeight,
                                               bold ? juce::Font::bold : juce::Font::plain));
    result.setFallbackEnabled(true);
    result.setPreferredFallbackFamilies(fallbacks(role));
    return result;
  }

  static juce::Font fromTypeface(const juce::Typeface::Ptr& typeface, float height,
                                 FontRole role = FontRole::Text) {
    if (typeface == nullptr) return make(height, role);
    auto font = juce::Font(juce::FontOptions(typeface).withHeight(
        std::isfinite(height) ? juce::jlimit(8.0f, 120.0f, height) : 14.0f));
    font.setFallbackEnabled(true);
    font.setPreferredFallbackFamilies(fallbacks(role));
    return font;
  }

  static juce::StringArray availableFamilies() {
    return juce::Font::findAllTypefaceNames();
  }

  // Render missing text with JUCE's Unicode text shaping rather than replacing
  // characters with '?' or guessing per-codepoint fonts (breaks emoji clusters).
  static void apply(juce::Label& label, float height, FontRole role = FontRole::Text,
                    const juce::String& requestedFamily = {}, bool bold = false) {
    label.setFont(make(height, role, requestedFamily, bold));
  }

private:
  static juce::String chooseFamily(FontRole role) {
    if (role == FontRole::Emoji)
      return firstInstalled({"Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji",
                             "Noto Emoji"});
    if (role == FontRole::Symbol)
      return firstInstalled({"Segoe UI Symbol", "Noto Sans Symbols 2", "Noto Sans Symbols",
                             "DejaVu Sans"});
    return juce::Font::getDefaultSansSerifFontName();
  }

  static juce::String firstInstalled(std::initializer_list<const char*> candidates) {
    const auto families = availableFamilies();
    for (const auto* family : candidates)
      if (families.contains(family, true)) return juce::String(family);
    return juce::Font::getDefaultSansSerifFontName();
  }

  static juce::StringArray fallbacks(FontRole role) {
    const auto families = availableFamilies();
    const std::vector<juce::String> preferred = role == FontRole::Emoji
      ? std::vector<juce::String>{"Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji",
                                   "Noto Emoji", "Segoe UI Symbol", "Noto Sans Symbols 2",
                                   "DejaVu Sans"}
      : std::vector<juce::String>{"Segoe UI Symbol", "Noto Sans Symbols 2",
                                   "Noto Sans Symbols", "Segoe UI Emoji", "Apple Color Emoji",
                                   "Noto Color Emoji", "Noto Sans", "DejaVu Sans"};
    juce::StringArray result;
    for (const auto& family : preferred)
      if (families.contains(family, true)) result.addIfNotAlreadyThere(family);
    return result;
  }
};
} // namespace skellcore::ui
