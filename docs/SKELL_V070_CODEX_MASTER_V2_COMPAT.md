# SkellCore v0.7 / Codex Master v2 compatibility branch

This draft branch combines two *already reviewed independently* development streams without overwriting either:

- Base: v0.7 DSP + FDN + shared UI v2 integration, [draft PR #31](https://github.com/skellyruno/skellcore-vst-template/pull/31) at `dbbbc3e24a7e22abdcd41cf5efe3152bc9d15403`.
- Additions: exact `master_v2` header, source regression test and provenance note from Codex's [draft PR #35](https://github.com/skellyruno/skellcore-vst-template/pull/35) at `fe2f413dc9860db32d270898b123a120d2599771`.
- CMake is **reconciled**, not copied: adds `SkellCoreMasterV2Tests` while retaining the v0.7 FDN, limiter parity, shared UI meter, host lifecycle, local tuning and other existing tests.

Existing `SkellCore/backends/master/MasteringEngine.h` stays untouched. The new header is opt-in under `skellcore::master_v2`. This is not a default backend switch and does not alter any plugin, DAW state, automation ID, ABI, generated default, or deployed package.

## Checks and release gates

- Verify native portable Windows + Linux test matrices and sanitizers against *this exact commit*. Confirm the new Master v2 test and existing FDN test both execute.
- Verify template Windows VST3/Standalone and shared UI renderer compatibility separately.
- Codex Master v2 snapshot expects an oversampled caller; no 4x oversampler or plugin parity is supplied here.
- SkellCore v0.7 limiter remains **not true-peak certified**; the known intersample over-ceiling issue is tracked in [issue #32](https://github.com/skellyruno/skellcore-vst-template/issues/32). Do not merge as a production mastering change.
- Keep PR draft until all gates pass and branch stacking conflicts are reviewed. Main and the original Codex PR #35 stay untouched.
