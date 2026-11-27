# Native sound matching: shared DSP, phase 1

SkellCore owns the production DSP implementation. Sound Match fits parameters by invoking the compiled native renderer; it never renders candidate audio using a separate Python implementation. Plugins pin an exact SkellCore revision and share the same header and processing function. This is an incremental migration, not replacement of existing native effects.

ClubGate.h preserves the original Skell Preset MIC gate and optional causal B finish. clubGateGain is the common processing path. It is allocation-free in processGain, stereo-linked, and adds no explicit lookahead delay. The B finish is a causal approximation of a previous centered offline envelope; the owner's earlier B listening approval does not automatically apply to this implementation.

Build from SkellCore:
    cmake -S SkellCore -B build-native -DSKELLCORE_BUILD_NATIVE_MATCH_RENDERER=ON -DSKELLCORE_BUILD_TESTS=ON
    cmake --build build-native --config Release
    ctest --test-dir build-native -C Release --output-on-failure

CLI transport: native-endian interleaved IEEE float32 stereo, one pair per frame. Input only; output must not already exist. Arguments:
    SkellCoreClubGateRender input.f32 output.f32 rate threshold attack hold release hysteresis finish inputDb outputDb block
The output gain is a standalone level calibration, not proof of full plugin routing parity. The executable prints frame count, processing seconds and algorithmic latency. The reference stays in the optimizer, never in this CLI.

Local targeted CMake build (GNU 14.2, Release, -Wall -Wextra -Werror) and 1/1 CTest passed, with 44.1/48/96/192 kHz opening/release/determinism/finite tests. The local build used the new targets in an isolated minimal CMake project, not the full core suite. Real Club raw/gate pair: 527928 frames at 44.1k, fit on first 60%, later section scoring only. Exact output equality for block sizes 1/64/256/511/1024. No new owner audition; full JUCE wrapper parity, CPU under host, independent takes and full CI remain required. No private audio/assets included.

Next effects: shared convolution and modulation (Wobbly/Muffle), stage-based delay (Angelic/Zaam/Speakerz), dynamics/EQ (Alchemist/Deuce/Hop Out). Before approval, recipe pins DSP commit + executable fingerprint + exact parameter values, routing, sample rate, authorized IR hashes, render hash and matching data. No silent fallback or automatic factory changes. Full VST3 parity must compare the actual wrapper with the standalone engine at the same level, routing and parameter schedule.
