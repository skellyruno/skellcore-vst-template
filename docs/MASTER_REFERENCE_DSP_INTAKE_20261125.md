# Master reference-model intake — 2026-11-25

**Preservation gate:** no production Master, core backend, factory preset or automation change. Owner approved V16 sound; verified V18 exact head 6da5d4340bd1a30382ca11f696663c15774e7647, Windows/Linux run 45631330044. Original private FST/audio not included. Research only.

## Intake and provenance

Eleven supplied extracts examined locally. Four active RealWorld states from original FL mixer FST decoded, SETUP_A. API-2500 mapping: +0.69 dB threshold, 3:1 ratio, 0.1ms attack, 0.5s release, medium Thrust, hard knee, feedback, manual +5.2dB makeup, wet100, trim0. Vitamin: 90/500/2594/9310Hz, enhancement 33.6/29.9/28.1/26.2/24dB, direct0dB. Supplied translator does not map punch or per-band width. L2 threshold -2.8dB, ceiling -1.1dB, ARC on, 24-bit, type1 dither/normal shaping. API-550A +2dB at 5kHz high shelf, bandpass off. These are source-state values and translator interpretations, not verified vendor coefficient laws.

Compressor wrapper depends on missing BusComp2500.h. Matching source found at https://github.com/Ka1-S/LYNX-SHARE/blob/adee2e6825c7199e4f7a74142330a69d37e03ef5/src/core/BusComp2500.h ; repository LICENSE AGPL-3.0. Supplied snippets match this project family with changed namespace/comments. Treat as third-party provenance until rights established. No imported code, fit tables, audio, preset files or vendor binaries in this PR. Test locally only. Do not combine AGPL-derived code into shared production plugins without license review/authorization. Other gaps: full Facet EQ magnitude-matched implementation missing from upload, StandardCLIP engine missing, limiter header has malformed opening comment, original wrappers depend on absent framework. Vitamin header compiled directly locally with C++20. Local adapter evaluates feedback bus fit, supplied Vitamin, approximate hard clip at -1.2dBFS or no clip, uploaded L2 ARC recurrence with scan oracle and two moving averages (no dither, 59 sample delay at 44.1k), optional single RBJ bell approximate API-550 high band (not full Facet or flat response). These are partial-chain challengers, NOT exact complete original plugin matches. No vendor code execution.

## Real music comparison

Same private owner song, 13-second excerpts starting 3/30/83s; actual hosted V18 factory5 4x render; use nominal reference PDC31 (prior recorded), V18 105, candidate59; exclude first/last 1s. Correlation peak of raw versus reference in two subwindows of 30s excerpt varied 268/243 samples due to processing; alignment not independently proven by impulse, so no waveform ESR/null claim for imported challenger. Integrated loudness difference in LU (candidate minus five-plugin reference); spectral five-band Welch mean absolute dB error and 20ms RMS envelope dB MAE after LU match; independent 8x polyphase peak estimate on unnormalized output. Not alias-only or listening scores. All excerpts same song, not independent material.

|Start seconds|Path|LU gap|Band MAE dB|20ms MAE dB|8x peak dBTP estimate|
|---|---|---|---|---|---|
|3|V18|-0.9477|0.4969|0.4585|-0.4748|
|3|hard clip, no EQ|-0.9536|1.4724|0.7693|+1.2391|
|3|hard clip + approximate EQ|-0.2552|0.7324|0.5197|+2.7622|
|3|no clip + approximate EQ|-0.5980|0.8453|0.6685|+2.6216|
|30|V18|-0.5488|0.1134|0.5383|-0.4617|
|30|hard clip, no EQ|+0.1514|2.6455|1.4019|+0.8869|
|30|hard clip + approximate EQ|+0.4633|1.8951|1.1530|+2.8142|
|30|no clip + approximate EQ|-2.2271|1.6196|1.0481|+1.0069|
|83|V18|-0.5827|0.1203|0.4558|-0.4714|
|83|hard clip, no EQ|+0.1069|2.6677|1.3396|+1.9737|
|83|hard clip + approximate EQ|+0.4049|1.9106|1.1093|+4.4518|
|83|no clip + approximate EQ|-2.2313|1.5749|0.9954|+0.8598|

Some loudness gaps improved; every scored challenger loses on tonal/envelope metrics and exceeds 0dBTP estimate. Do not promote. Source reference itself can exceed full scale in intersample reconstruction; retain safe ceiling rather than target these peaks. Missing StandardCLIP and exact EQ, gain staging and omitted width/punch/dither limit conclusions about individual engines.

## New independent reusable research primitive

SkellCore/experimental/ProgramRecovery.h (local verified blob 6a0bc2d6aeca251966068f4b5662a4541b99c244), original generic dual-memory bounded gain recovery, no imported calibration/code. Smooth required reduction with 65ms rise/280ms fall; blend 40ms and 220ms release coefficients by memory; immediate cap at caller-supplied permissible gain. This is an envelope utility, NOT a true-peak limiter or lookahead algorithm. All coefficient design in prepare; process no allocations, locks, I/O or exponentials. Test exact blob 36f82b05a4a98f1827f96d6611c0929630a2fa2d. C++17 optimized -O3 -Wall -Wextra -Werror and ASan/UBSan passed at 44.1/48/96/192/352.8/768k. After 100ms release from 70% attenuation, 5ms event recovers to .9313 linear gain versus .5562 after 500ms event. Random bound checks 100k per rate; reset, invalid input and unity tested. Research target only, no actual Master integration; no claim of sonic improvement. CMake adds one portable test; complete exact-head CI pending.

## Transfer roadmap

- VCA feedback/Thrust: independent optional detector stage for Glue, Skell Preset and rack compressors, measure detector weighting and actual gain trajectories, preserve reference path.
- Five-band phase compensation: optional residual/phase experiments for Sauce, multiband saturation and Width; compare exact direct sum and inspect transient phase, protect mono. Vitamin punch/width remain missing.
- Limiter: independent program recovery and moving-average lookahead research, preserve latency and true peak guard. Test safety separately from output requantization; no IDR integration until dedicated export research and licensing clear.
- EQ: measured response and proportional-Q research, use own coefficients; compare complete magnitude/phase with exact reference probes rather than inventing missing fit data.
- StandardCLIP: original soft-knee challenger separately measured at 2/4/8x, no supplied replacement engine.

Local JUCE source retrieval again DNS-blocked; no UI change or new screenshot. No new VST3 produced. No audio committed; no change to master/main. Draft PR only.
