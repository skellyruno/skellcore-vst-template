# Native matching convolution and ducking — 2026-11-24

Shared MatchConvolution implements uniform partitioned stereo FIR with explicit rate contract, dual-mono or mono-to-stereo routing, smoothed additive wet return, delayed dry alignment, declared partition latency, reset and finite input checks. Resampling of authorized IRs is performed off the audio thread before prepare; no private IR is included. No allocation in process; benchmark worst-block time and denormal handling before plugin adoption. Existing MatchStereoDelay now has optional causal wet-only ducking, threshold, attack/release and explicit output gain; defaults preserve previous recipes. Existing v1 CLI retains its old argument list (new controls exposed through the C++ API, not v1 transport).

Targeted GNU C++17 -O2 -Wall -Wextra -Werror tests passed: original delay regression, new ducking regression, convolution direct FIR parity across 44.1/48/96/192 kHz, partitions 64/256/1024, both routes and five block schedules. Local test blob identities verified before publishing. Actual authorized Muffle (44.1 kHz / 50782 frames) and Wobbly (48 kHz / 136241 frames, resampled with rate-ratio gain convention to 44.1 kHz) native FIR parity against independent FFT convolution: maximum absolute error 7.63e-6 and 2.98e-8 respectively. Local in-process elapsed 0.769/1.835 s for 11.97 s of audio at 256 partition; 5.805 ms dry alignment latency, not host CPU certification. Includes preparation and bridge overhead. Additive matching trials selected zero extra wet, so no improved Muffle/Wobbly replacement. Preserve prior owners' choices.

No production plugin or preset integration. Full core CI, Windows/VST3, worst callback time, host PDC, complete routing and independent listening remain open.

## Conjugate-symmetry optimization

Only unique spectral bins are accumulated; negative frequencies are restored before IFFT. Direct FIR tests rerun and passed. Identical real Club stage input, two alternating runs per version, GNU C++17 -O3, 11.971 s at 44.1 kHz, native bridge including setup, elapsed median:

| IR | Partition | Before s | After s | Output max difference |
|---|---:|---:|---:|---:|
| Muffle | 256 | 0.7752 | 0.4256 | 0 |
| Muffle | 512 | 0.3997 | 0.2324 | 0 |
| Muffle | 1024 | 0.2395 | 0.1494 | 0 |
| Wobbly | 256 | 1.8165 | 0.9969 | 0 |
| Wobbly | 512 | 0.9088 | 0.5108 | 0 |
| Wobbly | 1024 | 0.5023 | 0.2892 | 0 |

Approximately 45% reduction in elapsed time at partition 256; actual max absolute FIR discrepancy remains 7.63e-6 / 2.98e-8. Local average render timing does not certify worst-block deadlines, DAW CPU, denormal handling or PDC. Longer partitions trade latency for throughput. No production changes.
