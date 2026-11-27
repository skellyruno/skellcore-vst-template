# Opt-in presence wet-path trim

Transferred the independent real-vocal research from skell-vst-decompiler commit a647e3c6 into an explicitly optional SkellCore experimental component. PresenceWetTrim wraps the existing PresenceEngine without modifying the engine or its default settings. Zero trim delegates directly to the original engine; nonzero trim scales only the processed path before mixing with the dry input. Preparation only, no runtime parameter smoothing or additional delay/allocations. Invalid nonfinite controls use the existing bounded fallbacks. This is not a limiter.

Local exact-header regression: optimized GCC and ASan/UBSan passed at 44.1/48/96/192 kHz, 240,000 stereo frames across 12 rate/mix configurations, reset, NaN/Inf input and invalid trim. Added the test to the portable CMake test list; full hosted CMake/CI not yet verified.

Actual private vocal replay: 36 excerpts at 44.1 kHz, 108 mix cases (0/.5/1) and -4 dB trim. All zero-trim comparisons and expected blend samples exact, 36 unchanged dry endpoints. Maximum estimated peak: dry -1.352 -> -1.352 dBFS; .5 mix -.780 -> -3.436; full wet +2.316 -> -1.684. No output samples over full scale in tested trimmed cases. Peak estimation uses 4x polyphase interpolation, not certified true peak. Reproduce locally with `python SkellCore/tools/bench_presence_vocals.py --manifest /private/manifest.json --output /private/metrics.json` (NumPy/SciPy/SoundFile/GCC). Private files, names and per-input results not committed. Local report hash in adjacent JSON.

No production plugin or preset is automatically changed. Adoption requires an approved mix/gain decision and plugin-specific host tests. At partial mix a wet trim changes the wet/dry balance by design.
