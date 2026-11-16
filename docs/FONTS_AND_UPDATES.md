# SkellCore shared fonts and version checks

SkellCore provides four optional components: `FontSystem.h` (font family selection with Unicode/emoji fallback), `Version.h` (numeric versions), **`ReleasePolicy.h`** (portable deterministic update decision), and `UpdateBadge.h` (click-to-check stable GitHub releases).

## Fonts, symbols, emoji

Use FontSystem::make(height, role, requestedFamily, bold), FontSystem::apply(label,...) or FontSystem::fromTypeface(typeface,height,role). Requested licensed families are preserved. JUCE 8 per-glyph fallback remains enabled, and installed native emoji/symbol fonts are preferred. Missing families fall back via JUCE; no font can guarantee every glyph on every computer. Coloured emoji and complex sequences depend on installed fonts and OS support; prefer vector icons for critical UI arrows, power and settings. Font binaries must be licensed. Never scan fonts or allocate UI font resources on the audio thread.

Example: include <SkellCore/FontSystem.h> and use skellcore::ui::FontSystem::apply(title, 22, skellcore::ui::FontRole::Heading, "My Licensed Font", true). For emoji use FontSystem::make(18, FontRole::Emoji); always preserve UTF-8 input.

## Version update badge

Create skellcore::ui::UpdateBadge("Skell Preset", PROJECT_PLUGIN_VERSION, "skellyruno", "skell-preset-vst") in the real editor. Place it in resized(), then provide the owning plugin's build version/release repository (do not reuse SkellCore version in every product). Clicking the version checks the GitHub `releases/latest` stable-release API in an **owned worker**, not at startup and never in `processBlock()`. The worker is joined during badge destruction before plug-in unload, rather than detached with code still running in the host; closing the editor while a slow request is active may wait for network I/O to return. Responses are limited to 128 KiB and timeouts are requested for connection setup. A newer stable version triggers Get update / Later confirmation. Get update opens the validated GitHub HTTPS release page; it never replaces loaded binaries, installs, or edits presets/automation. No published release, unauthenticated/private responses, rate limits, malformed or draft/prerelease tags, API errors, and offline conditions produce distinct, honest outcomes. The policy never invents an available release or downloads any executable.

Only verified stable releases should be published. Self-installing an in-use VST3 is unsafe; any future installer requires separate signed packaging, DAW shutdown, backup and rollback policy.

## Verification

Portable: `cmake -S SkellCore -B build-core -DSKELLCORE_BUILD_TESTS=ON; cmake --build build-core; ctest --test-dir build-core --output-on-failure -R 'SkellCore(Version|ReleasePolicy)Tests'`. Both suites use unconditional `CHECK` assertions that are active even in Release/NDEBUG. The release policy covers offline/401/403/404/429/502 responses; missing, draft, prerelease, path-injection and invalid tags; short versions, downgrade and newer releases.

JUCE/host gates: build real Windows VST3 and standalone, inspect Quick4 real-editor screenshots for small/default/large layouts and at 125/150/200% scaling, and exercise emoji/symbol display plus version badge on offline, 404, current, newer, and editor-close-during-check paths. FL Studio host is a separate gate. Existing plugins opt in only by pinning a verified SkellCore commit.

Windows source encoding: the MSVC plugin and renderer targets now compile with `/utf-8`. Cross-platform Unicode test glyphs use C++ universal-character-name escapes (for example `\\u2713`) so their bytes are not accidentally decoded through a legacy Windows code page before JUCE receives them. Font fallback is separate from correct UTF-8 decoding: installing more fonts cannot repair already-corrupted source literals. Verify the actual `unicode_ivory_large.png` screenshot after each change.

## Thread lifetime and release safety

An editor owns its network worker and receives updates via `juce::AsyncUpdater` on the UI thread. Destruction joins the worker and cancels queued updates, preventing detached work from continuing after a plug-in unload. No network work happens on audio callbacks. The remote tag is never treated as a URL; GitHub release pages are constructed from locally configured, strictly validated owner/repository and tag parts. The update choice only opens an official HTTPS GitHub release page, never downloads or overwrites a loaded VST3. The JUCE asynchronous confirmation dialog still requires an actual host-level lifecycle test when a DAW editor is closed during the prompt. Do not market automatic installation or FL Studio certification until those checks pass.

## All portable tests are meaningful in Release builds

`SkellCore/CMakeLists.txt` removes the default `/DNDEBUG` or `-DNDEBUG` definition from the **SkellCore portable-test directory's Release flags** when tests are enabled, leaving production plug-in compilation untouched and avoiding MSVC D9025 override warnings. A `SkellCoreAssertionsActiveTests` compile gate fails if `NDEBUG` remains defined. The historical tests using `assert(...)` now actually exercise their checks on Windows/Linux Release builds; new release and version tests also use unconditional CHECKs. Do not remove the guard without replacing all assertions with unconditional test checks.
