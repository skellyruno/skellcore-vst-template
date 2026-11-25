# SkellCore source provenance (2026-11-07)

Original production DSP preserved in SkellCore. Imported source hashes are listed below for reproducible review.

| Upstream component | Original GitHub blob SHA |
| --- | --- |
| SkellyTune RealtimeTuner.hpp | `b3e118fd76533f74b19a4e0c28b1e5f34c403dd2` |
| SkellyTune DelayContract.hpp | `5b0e15ed405f5ab8a2a95779d7e9bc690459382c` |
| SkellyTune CausalCycleSynth.hpp | `ac7877b86414c0f53623f4a021575649aeee6732` |
| SkellyTune FormantEnvelopeGuard.hpp | `cef1e05f61136c5d636e4b82b6e196d7698ebde0` |
| DeEsser PhraseAwareDetector.h | `7337cfcb1ec299128336f7d24fbae4965af6fe2f` |
| Skelly Master MasteringEngine.h | `a2b2264815e07647422864bcdd75a17b63695e50` |

- SkellyTune real-time pitch and related DSP: `skellyruno/skelltune-mini`, `dsp/*.hpp` at commit around v3.3. License: MIT (copied in `licenses/SkellyTune-MIT.txt`). The backend is *not* a new pitch algorithm; it adapts this implementation.
- Phrase-aware de-esser: `skellyruno/skell-deesser-vst`, `Source/dsp/PhraseAwareDetector.h`. The file is copied with only the enclosing namespace changed from global to `skellcore::deesser` to avoid collisions.
- Mastering engine: `skellyruno/skell-master-vst`, `Source/DSP/MasteringEngine.h`. Copied with an enclosing namespace change from `skelly` to `skellcore::master`, avoiding symbol conflicts with SkellyTune.

When mixing dependencies from other authors or plugins, verify their redistribution and commercial-use terms *before import*. Do not assume all GitHub repos are permissively licensed.

Known quality limits:
- The prototype `TruePeakLimiter` is **not** certified true-peak or BS.1770 compliant.
- The production mastering DSP expects a separate oversampler.
- The production pitch source may have variable source age for voiced synthesis; the delay contract describes the fixed dry/transient host-compensated path, not time-varying source grain age.
- The header-only `SkellyTuneBackend` is currently realtime profile; no experimental cycle-synthesis mode is enabled by the adapter.

## Original wet duck recovery candidate — 2026-11-24

Independent Skell design based on the previously documented OriginalSpace weakness (slow wet recovery); no vendor code, tables or private audio transferred. Gain-domain attack/release retains the old steady-state rational target, with no allocation, locks or math transcendental calls in process(). This is an optional header, not a change to existing effects. Construct off the audio thread. No parameter automation API yet.

Local g++ C++17 -O3 -Wall -Wextra -Werror and ASan/UBSan passed at 44.1/48/96/192 kHz. Constant stereo input (.5,-.2), 2 seconds, followed by 3 seconds silence, depth 9 dB, attack 14 ms, release 260 ms. Active gain 0.462344 for both, old t90 1041.91–1041.93 ms, new t90 598.677–598.688 ms (90% recovery of gain deficit, not reverb RT60). Stereo swap, zero depth, reset, monotonic release, nonfinite input, invalid sample rate checked. Synthetic gain controller test only; faster attack/release may affect transients or pumping. No real audio, full FDN comparison, plugin CPU/latency, Windows VST3, Quick4, pluginval or FL Studio tested. Production DSP, UI, states and automation remain unchanged. Local JUCE missing; GitHub clone DNS failed. Next: feed identical owned FDN wet stream through both controllers, then independent real vocal/music A/B, peak and modulation analysis, before optional integration.

Locally tested Git blob IDs: header 90658e9ff2900c39e317469beb40d432bcffa8a4; test a19eb8e8c3cce47352e8a7b91313c7a15b54ae24.
