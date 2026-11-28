#pragma once
#include <algorithm>
#include <cmath>

namespace skellcore::experimental {
// Independent dual-memory release candidate. Input is the maximum permitted
// linear gain for the current frame (lookahead supplied by caller). The
// output never exceeds that bound. No buffer, lookahead, dither or makeup.
// This is not a complete limiter and does not guarantee reconstructed peaks.
class ProgramRecovery {
public:
    void prepare(double sampleRate) noexcept {
        const double rate = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000., 768000.) : 48000.;
        const auto pole = [rate](double seconds) { return -std::expm1(-1. / (rate * seconds)); };
        memoryRise = pole(.065); memoryFall = pole(.280);
        fast = pole(.040); slow = pole(.220);
        reset();
    }
    void reset() noexcept { memory = 0.; gain = 1.; }
    double process(double limit) noexcept {
        limit = std::isfinite(limit) ? std::clamp(limit, 0., 1.) : 0.;
        const double demand = 1. - limit;
        memory += (demand - memory) * (demand > memory ? memoryRise : memoryFall);
        const double blend = std::clamp(memory * 2., 0., 1.);
        const double release = fast + blend * (slow - fast);
        gain = std::min(limit, gain + release * (limit - gain));
        if (limit == 1. && gain > 1. - 1.e-12) gain = 1.;
        return gain;
    }
private:
    double memory = 0., gain = 1.;
    double memoryRise = 0., memoryFall = 0., fast = 0., slow = 0.;
};
}
