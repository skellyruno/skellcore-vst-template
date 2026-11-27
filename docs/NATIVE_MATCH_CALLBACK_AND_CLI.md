# Native matching callback optimization and Release CI follow-up

Previous PR head 12de70f: Windows portable tests, Linux ASan/UBSan, headless JUCE and template Windows VST3 succeeded; Linux Release CTest 16/17 failed (MatchConvolutionTests abort without diagnostic, GNU 15.2, run 45479347055). Old source passed local GNU 14.2 and Clang Release, including native FMA flags, so exact cause is unconfirmed. Keep original tolerance and exact equality checks. Use a single render loop for schedule comparisons and report assertion line/expression. No failure suppression.

Compact spectra: store only p+1 bins per partition, reuse one preallocated FFT scratch per channel, explicit real/imaginary multiply to avoid complex fallback. Approximate spectral storage halved (p+1 vs 2p; small extra scratch). New source passed GCC and Clang -O3 -DNDEBUG, default and -march=native, -Wall -Wextra -Werror. All three targeted GCC executables passed. Actual Club gate input and original Muffle/Wobbly IRs (Wobbly resampled as before): old/new output maximum absolute difference zero at partitions 256/512/1024; independent FFT FIR error unchanged (7.63e-6 and 2.98e-8).

Callback benchmark: AMD EPYC 9V74, GNU 14.2 -O3, 44.1k, 527928 input frames, real IRs, one render per partition/block combination. Processing only, not plugin host, includes scheduling noise; mean/p99/max in microseconds. At partition 256/block 64:
- Muffle old 43.505/253.472/997.565, new 23.549/162.516/1367.393; no deadlines missed in either.
- Wobbly old 100.443/517.080/9435.545 (3 missed), new 46.280/261.875/1172.379 (0 missed).
- Wobbly partition 256/block 32 still missed 7 deadlines versus 13 previously. Do not claim real-time safe for all buffers. Longer partition tradeoff and scheduling need testing. Peak time noisy; no host CPU certificate.
- Partition 512 Wobbly new zero deadline misses across tested blocks 32/64/128/256 in this single pass; latency 512 samples, not default/promotion.

Standalone delay renderer accepts optional five controls after the existing block argument: duckDepth duckThreshold outputGain duckAttackMs duckReleaseMs. --version stays v1 for legacy clients; --capabilities advertises extended syntax. Actual Club audio legacy/default optional args sample-exact; extended duck recipe exact at 1/64/256/511/1024 blocks. Overwrite and out-of-range duck depth rejected. No approved preset modified; no private audio/IR in public source.

Local JUCE retrieval failed DNS. New-head CI, native wrapper parity, independent listening, FL Studio and exact Deuce v2.5 remain open. Previous release failure not declared solved until CI result.
