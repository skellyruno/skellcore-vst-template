#pragma once
// Shared opt-in font helpers for JUCE 8 editor code. Never call on the audio thread.
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <initializer_list>

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
    return installedFamilies();
  }

  // Render missing text with JUCE's Unicode text shaping rather than replacing
  // characters with '?' or guessing per-codepoint fonts (breaks emoji clusters).
  static void apply(juce::Label& label, float height, FontRole role = FontRole::Text,
                    const juce::String& requestedFamily = {}, bool bold = false) {
    label.setFont(make(height, role, requestedFamily, bold));
  }

private:
  // Cache platform enumeration: makes recurring editor repaints independent
  // of costly installed-font scans. Loaded plugin typefaces still work by name.
  static const juce::StringArray& installedFamilies() {
    static const auto installed = juce::Font::findAllTypefaceNames();
    return installed;
  }

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
    const auto& families = installedFamilies();
    for (const auto* family : candidates)
      if (families.contains(family, true)) return juce::String(family);
    return juce::Font::getDefaultSansSerifFontName();
  }

  // Immutable OS font fallback tables, built once on the UI thread. Previous
  // implementation allocated a vector and StringArray on *every paint*.
  static juce::StringArray detectFamilies(std::initializer_list<const char*> candidates) {
    const auto& installed = installedFamilies();
    juce::StringArray selected;
    for (const auto* name : candidates)
      if (installed.contains(name, true))
        selected.addIfNotAlreadyThere(name);
    return selected;
  }

  static const juce::StringArray& fallbacks(FontRole role) {
    static const auto emoji = detectFamilies({
      "Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji", "Noto Emoji",
      "Segoe UI Symbol", "Noto Sans Symbols 2", "DejaVu Sans"});
    static const auto general = detectFamilies({
      "Segoe UI Symbol", "Noto Sans Symbols 2", "Noto Sans Symbols",
      "Segoe UI Emoji", "Apple Color Emoji", "Noto Color Emoji",
      "Noto Sans", "DejaVu Sans"});
    return role == FontRole::Emoji ? emoji : general;
  }
};
} // namespace skellcore::ui
