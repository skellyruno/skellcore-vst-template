# SKELL reusable Egyptian rune library

Reusable **24 original** Egyptian-inspired signs (ankhs, eyes, scarabs, birds,
cobras, cartouches, feathers, staffs, reeds and more) from the verified
Skell-Necromancer glyph upgrade. Existing plugin glyphs 0–39 are preserved.

- `<SkellCore/RuneGlyphCatalog.h>`: portable 64-slot catalogue (40 old/24 new).
- `<SkellCore/RuneGlyphArtwork.h>`: JUCE GUI-only vector artwork, one-time
  `extendedAtlas(legacyAtlas)` or `egyptianOnlyAtlas()`.
- `<SkellCore/RuneOrbit.h>`: portable 3-band orbit, fade and 30 FPS pacing.

```cpp
#include <SkellCore/RuneGlyphArtwork.h>
auto combined = skellcore::ui::runes::extendedAtlas(existingAtlas);
```

Create/cache atlas and sprites **once in the editor**, not in paint or the audio
callback. Other plugins can choose their own accent colors, mix and geometry.
No font, emoji, external asset or network dependency is required. These
components are opt-in: no existing plugin state, audio engine or automation is
modified. Pin the SkellCore commit before consuming in another plugin.

Provenance: original vectors authored for Necromancer commit
`5b7ba2b3d094f9b144d72cce6dbb9cac92369e51`.
Portable CTest covers the catalogue and motion; JUCE drawing and real Quick4
screenshots still require a JUCE-enabled project and visual verification.

## Independent real-JUCE compile gate

The optional `SkellCoreUISmokeRenderer` target includes the shared artwork
header, creates an actual JUCE `Image`, and checks all 24 Egyptian cells for
non-empty anti-aliased strokes. The renderer keeps the same **13** real editor
scenes so existing Quick4 screenshot count/manifest requirements do not change.
This validates the portable library *and* the JUCE drawing interface separately.
