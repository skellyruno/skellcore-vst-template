# Opt-in Master v2 DSP snapshot

Source: skellyruno/skell-master-vst `ca3166f107e609f8cbb442429cf25a7a5c8a256f`.
Header: `SkellCore/backends/master_v2/MasteringEngine.h`.
Namespace: `skellcore::master_v2`. Only namespace and provenance comments differ
from the source engine. The source regression test is included with its include
and namespace adapted. Existing `skellcore::master` is unchanged.

This retains the newer drive shaper, finite-input handling, stereo imager and
sample-peak safety guard as an explicitly selected backend. It does not change
any existing plugin, parameter, preset, or default sound.

Windows x64 Release portable build and all 13 CTest targets passed on 2026-11-15.
The new Master test uses explicit runtime requirements, including impulse,
limiter, automation, mono/stereo, invalid inputs and a CPU sanity probe.
Older Core tests also contain assertions; Release results alone are not a full
assertion-enabled audit. This is a portable inner engine, not a native plugin.

The engine expects properly oversampled input. The reference plugin still uses
JUCE's 4x oversampling wrapper. No replacement oversampler or Master/Preset VST3
port is included. Complete plugin parity, aliasing/true-peak measurement, UI,
legacy state and FL Studio testing remain separate work, deferred by request.
