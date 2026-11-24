#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace skellcore::dsp {
// Native matching primitive: additive wet return, never cancellation of a full mix.
// Prepare/reset off the audio thread. process() has no allocation or parameter fitting.
class MatchStereoDelay final {
public:
    struct Frame { float left = 0.f, right = 0.f; };
    struct Parameters {
        double delayMs = 460., cutoffHz = 5000.;
        float feedback = .5f, cross = 0.f, wet = .25f;
        float midGain = 1.f, sideGain = 1.f;
    };
    bool prepare(double rate, Parameters p) {
        if (!std::isfinite(rate) || rate < 8000. || rate > 192000. ||
            !std::isfinite(p.delayMs) || p.delayMs < 1. || p.delayMs > 2000. ||
            !std::isfinite(p.cutoffHz) || p.cutoffHz < 20. || p.cutoffHz >= rate * .5 ||
            !valid(p.feedback, -.95f, .95f) || !valid(p.cross, 0.f, 1.f) ||
            !valid(p.wet, 0.f, 2.f) || !valid(p.midGain, 0.f, 4.f) || !valid(p.sideGain, 0.f, 4.f)) return false;
        const auto length = static_cast<std::size_t>(std::llround(rate * p.delayMs * .001));
        std::vector<Frame> replacement(length);
        buffer.swap(replacement);
        params = p;
        alpha = static_cast<float>(1. - std::exp(-6.283185307179586 * p.cutoffHz / rate));
        wetAlpha = static_cast<float>(1. - std::exp(-1. / (.01 * rate)));
        reset();
        return true;
    }
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), Frame{});
        position = 0; low = {}; wetGain = targetWet = params.wet;
    }
    // Same audio thread as process(); delay time/routing remain frozen per recipe.
    bool setWet(float value) noexcept {
        if (!valid(value, 0.f, 2.f)) return false;
        targetWet = value; return true;
    }
    Frame process(Frame input) noexcept {
        input = {clean(input.left), clean(input.right)};
        if (buffer.empty()) return input;
        const auto tap = buffer[position];
        low.left = clean(low.left + alpha * (tap.left - low.left));
        low.right = clean(low.right + alpha * (tap.right - low.right));
        const float a = 1.f - params.cross, b = params.cross;
        buffer[position] = {clean(input.left + params.feedback * (a * low.left + b * low.right)),
                            clean(input.right + params.feedback * (a * low.right + b * low.left))};
        if (++position == buffer.size()) position = 0;
        wetGain += wetAlpha * (targetWet - wetGain);
        if (std::abs(targetWet - wetGain) < 1.e-7f) wetGain = targetWet;
        const float mid = .5f * (input.left + input.right) * params.midGain;
        const float side = .5f * (input.left - input.right) * params.sideGain;
        return {clean(mid + side + wetGain * low.left), clean(mid - side + wetGain * low.right)};
    }
    std::size_t delaySamples() const noexcept { return buffer.size(); }
    static constexpr int latencySamples() noexcept { return 0; } // dry path is immediate
private:
    static bool valid(float x, float lo, float hi) noexcept { return std::isfinite(x) && x >= lo && x <= hi; }
    static float clean(float x) noexcept { return std::isfinite(x) && std::abs(x) >= 1.e-20f ? x : 0.f; }
    Parameters params{};
    std::vector<Frame> buffer;
    std::size_t position = 0;
    Frame low{};
    float alpha = 0.f, wetAlpha = 0.f, wetGain = 0.f, targetWet = 0.f;
};
} // namespace skellcore::dsp
