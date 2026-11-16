# SkellCore shared fonts and version checks

SkellCore adds three optional editor helpers: FontSystem.h (font family selection with Unicode/emoji fallback), Version.h (portable numeric SemVer comparison), UpdateBadge.h (click-to-check stable GitHub releases).

## Fonts, symbols, emoji

Use FontSystem::make(height, role, requestedFamily, bold), FontSystem::apply(label,...) or FontSystem::fromTypeface(typeface,height,role). Requested licensed families are preserved. JUCE 8 per-glyph fallback remains enabled, and installed native emoji/symbol fonts are preferred. Missing families fall back via JUCE; no font can guarantee every glyph on every computer. Coloured emoji and complex sequences depend on installed fonts and OS support; prefer vector icons for critical UI arrows, power and settings. Font binaries must be licensed. Never scan fonts or allocate UI font resources on the audio thread.

Example: include <SkellCore/FontSystem.h> and use skellcore::ui::FontSystem::apply(title, 22, skellcore::ui::FontRole::Heading, "My Licensed Font", true). For emoji use FontSystem::make(18, FontRole::Emoji); always preserve UTF-8 input.

## Version update badge

Create skellcore::ui::UpdateBadge("Skell Preset", PROJECT_PLUGIN_VERSION, "skellyruno", "skell-preset-vst") in the real editor. Place it in resized(), then provide the owning plugin's build version/release repository (do not reuse SkellCore version in every product). Clicking the version checks the GitHub releases/latest stable-release API on a worker thread, not startup or processBlock(). A newer SemVer release triggers Get update / Later confirmation. Get update opens the validated GitHub HTTPS release page; it never replaces loaded binaries, installs, or edits presets/automation. No published release, malformed tags, API errors, and offline conditions are reported honestly.

Only verified stable releases should be published. Self-installing an in-use VST3 is unsafe; any future installer requires separate signed packaging, DAW shutdown, backup and rollback policy.

## Verification

Portable: cmake -S SkellCore -B build-core -DSKELLCORE_BUILD_TESTS=ON; cmake --build build-core; ctest --test-dir build-core --output-on-failure -R SkellCoreVersionTests.

JUCE/host gates: build real Windows VST3 and standalone, inspect Quick4 real-editor screenshots for small/default/large layouts and at 125/150/200% scaling, and exercise emoji/symbol display plus version badge on offline, 404, current, newer, and editor-close-during-check paths. FL Studio host is a separate gate. Existing plugins opt in only by pinning a verified SkellCore commit.
