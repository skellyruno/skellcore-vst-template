#ifdef NDEBUG
#undef NDEBUG
#endif
#include <SkellCore/Core.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr int kReferenceTaps = 64;
constexpr int kReferencePhases = 16;

struct ReferenceTruePeak {
    std::array<std::array<double, kReferenceTaps>, kReferencePhases> coefficients {};

    ReferenceTruePeak() {
        for (int phase = 0; phase < kReferencePhases; ++phase) {
            const double fraction = static_cast<double>(phase) / kReferencePhases;
            double sum = 0.0;
            for (int tap = 0; tap < kReferenceTaps; ++tap) {
                const double offset = static_cast<double>(tap - (kReferenceTaps / 2 - 1)) - fraction;
                const double sinc = std::abs(offset) < 1.0e-12
                    ? 1.0 : std::sin(kPi * offset) / (kPi * offset);
                const double angle = 2.0 * kPi * tap / (kReferenceTaps - 1);
                const double window = 0.42 - 0.5 * std::cos(angle) + 0.08 * std::cos(2.0 * angle);
                coefficients[static_cast<std::size_t>(phase)][static_cast<std::size_t>(tap)] = sinc * window;
                sum += sinc * window;
            }
            for (double& value : coefficients[static_cast<std::size_t>(phase)])
                value /= sum;
        }
    }

    double measure(const std::vector<float>& samples) const {
        if (samples.size() < static_cast<std::size_t>(kReferenceTaps)) return 0.0;
        double peak = 0.0;
        const int firstCenter = kReferenceTaps / 2 - 1;
        const int lastCenter = static_cast<int>(samples.size()) - kReferenceTaps / 2 - 1;
        for (int center = firstCenter; center <= lastCenter; ++center) {
            const int firstSample = center - (kReferenceTaps / 2 - 1);
            for (int phase = 0; phase < kReferencePhases; ++phase) {
                double value = 0.0;
                const auto& coeff = coefficients[static_cast<std::size_t>(phase)];
                for (int tap = 0; tap < kReferenceTaps; ++tap) {
                    value += coeff[static_cast<std::size_t>(tap)] *
                             samples[static_cast<std::size_t>(firstSample + tap)];
                }
                peak = std::max(peak, std::abs(value));
            }
        }
        return peak;
    }
};

bool runCase(int sampleRate, double frequency, skellcore::TruePeakLimiter::Quality quality,
             const ReferenceTruePeak& reference) {
    skellcore::TruePeakLimiter limiter;
    limiter.prepare(static_cast<double>(sampleRate), 2, quality);
    limiter.setCeilingDb(-1.0f);
    const int latency = limiter.latencySamples();
    const int expectedLatency = std::max(48, static_cast<int>(sampleRate * 0.003));
    if (latency != expectedLatency) {
        std::cerr << "Latency changed: rate=" << sampleRate << " got=" << latency
                  << " expected=" << expectedLatency << "\n";
        return false;
    }

    const int inputSamples = sampleRate / 5; // 200 ms; enough repeated transients for every rate.
    const int outputSamples = inputSamples + latency + kReferenceTaps;
    std::vector<float> output(static_cast<std::size_t>(outputSamples), 0.0f);
    double samplePeak = 0.0;

    for (int i = 0; i < outputSamples; ++i) {
        float sample = 0.0f;
        if (i < inputSamples) {
            sample = static_cast<float>(1.2 * std::sin(2.0 * kPi * frequency * i / sampleRate));
            if (i % 887 == 0) sample = 3.0f;
        }
        const float input[2] {sample, sample};
        float result[2] {};
        limiter.processFrame(input, result, 2);
        if (!std::isfinite(result[0]) || !std::isfinite(result[1])) {
            std::cerr << "Non-finite output: rate=" << sampleRate << " frequency=" << frequency << "\n";
            return false;
        }
        if (result[0] != result[1]) {
            std::cerr << "Stereo mismatch: rate=" << sampleRate << " frequency=" << frequency << "\n";
            return false;
        }
        output[static_cast<std::size_t>(i)] = result[0];
        samplePeak = std::max(samplePeak, std::abs(static_cast<double>(result[0])));
    }

    const double ceiling = std::pow(10.0, -1.0 / 20.0);
    if (samplePeak > ceiling * 1.00001) {
        std::cerr << "Sample peak exceeded ceiling: rate=" << sampleRate << " frequency=" << frequency
                  << " peak=" << samplePeak << "\n";
        return false;
    }

    const double reconstructedPeak = reference.measure(output);
    const double overshootDb = 20.0 * std::log10(reconstructedPeak / ceiling);
    const double allowedOvershootDb =
        quality == skellcore::TruePeakLimiter::Quality::Standard ? 0.03 : 0.11;
    if (overshootDb > allowedOvershootDb) {
        std::cerr << "Inter-sample regression: rate=" << sampleRate << " frequency=" << frequency
                  << " quality=" << (quality == skellcore::TruePeakLimiter::Quality::Standard ? "Standard" : "Economy")
                  << " overshoot=" << overshootDb << " dB, allowed=" << allowedOvershootDb << " dB\n";
        return false;
    }

    std::cout << sampleRate << " Hz " << frequency << " Hz "
              << (quality == skellcore::TruePeakLimiter::Quality::Standard ? "Standard" : "Economy")
              << ": reconstructed overshoot " << overshootDb << " dB\n";
    return true;
}
} // namespace

int main() {
    const ReferenceTruePeak reference;
    for (int sampleRate : {44100, 48000, 96000}) {
        const std::array<double, 3> frequencies {{
            7000.0, 13517.0, 0.435 * static_cast<double>(sampleRate)
        }};
        for (double frequency : frequencies) {
            for (auto quality : {skellcore::TruePeakLimiter::Quality::Economy,
                                 skellcore::TruePeakLimiter::Quality::Standard}) {
                if (!runCase(sampleRate, frequency, quality, reference)) return 1;
            }
        }
    }
    std::cout << "PASS: independent 16-phase / 64-tap inter-sample regression matrix\n";
    std::cout << "NOTE: this regression reference is not a BS.1770 certification test.\n";
    return 0;
}
