#pragma once
// Original Skell experiment, based on public stacked-flanger behavior only.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace skellcore::experimental::modulation {
struct FlangeSettings {
    int order = 4;
    double delayMs = 4, depthMs = 2, rateHz = .25, spread = .6;
    double cross = 0, dry = 1, wet = .5;
};
class StackedFlanger {
public:
    static constexpr int maxVoices = 16;
    bool prepare(double sampleRate) {
        if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 192000) return false;
        fs = sampleRate;
        // Max 40 ms nominal delay plus spread and 20 ms modulated depth.
        const auto n = static_cast<std::size_t>(std::ceil(.12 * fs)) + 8;
        for (auto& channel : buffer) channel.assign(n, 0.0);
        setParameters(FlangeSettings{}); current = target; reset();
        alpha = -std::expm1(-1.0 / (.02 * fs));
        return true;
    }
    void reset() noexcept {
        for (auto& channel : buffer) std::fill(channel.begin(), channel.end(), 0.0);
        pos = 0; phase.fill(0.0); current = target; voiceCount = target.order;
    }
    // Call on the processing thread or before processing, not concurrently.
    void setParameters(FlangeSettings p) noexcept {
        auto clip = [](double v, double lo, double hi, double fallback) {
            return std::isfinite(v) ? std::clamp(v, lo, hi) : fallback;
        };
        p.order = std::clamp(p.order, 1, maxVoices);
        p.delayMs = clip(p.delayMs, 0, 40, 4); p.depthMs = clip(p.depthMs, 0, 20, 2);
        p.rateHz = clip(p.rateHz, 0, 5, .25); p.spread = clip(p.spread, 0, 1, .6);
        p.cross = clip(p.cross, -1, 1, 0); p.dry = clip(p.dry, -1, 1, 1); p.wet = clip(p.wet, -1, 1, .5);
        target = p;
    }
    void process(float& left, float& right) noexcept {
        if (buffer[0].empty()) return;
        const auto clean = [](float x) { return std::isfinite(x) ? std::clamp(static_cast<double>(x), -64.0, 64.0) : 0.; };
        const std::array<double,2> in{clean(left),clean(right)};
        for (int ch=0; ch<2; ++ch) buffer[ch][pos]=in[ch];
        auto smooth = [this](double& a, double b) { a += alpha*(b-a); };
        smooth(current.delayMs,target.delayMs); smooth(current.depthMs,target.depthMs);
        smooth(current.rateHz,target.rateHz); smooth(current.spread,target.spread);
        smooth(current.cross,target.cross); smooth(current.dry,target.dry); smooth(current.wet,target.wet);
        smooth(voiceCount,static_cast<double>(target.order));
        std::array<double,2> wet{}; double weights=0.;
        constexpr double twoPi=6.2831853071795864769;
        for (int i=0; i<maxVoices; ++i) {
            // Fixed voice layout; fractional activation smooths order changes.
            const double u = (2.0*i/(maxVoices-1))-1.0;
            const double w=std::clamp(voiceCount-i,0.,1.);
            const double speed=current.rateHz*(1.+.4*current.spread*u);
            phase[i]+=speed/fs; phase[i]-=std::floor(phase[i]);
            if (w==0.) continue;
            weights+=w;
            for (int ch=0; ch<2; ++ch) {
                const double angle=twoPi*(phase[i]+current.spread*(.37*i+.25*ch));
                const double delay=current.delayMs*(1.+.35*current.spread*u)
                    +current.depthMs*(1.+.25*current.spread*u)*(.5+.5*std::sin(angle));
                wet[ch]+=w*read(ch,delay*.001*fs);
            }
        }
        for (auto& w:wet) w/=std::max(1.,weights);
        const double cross=current.cross, own=1.-std::abs(cross);
        left=static_cast<float>(current.dry*in[0]+current.wet*(own*wet[0]+cross*wet[1]));
        right=static_cast<float>(current.dry*in[1]+current.wet*(own*wet[1]+cross*wet[0]));
        if (++pos==buffer[0].size()) pos=0;
    }
private:
    double read(int ch,double delay) const noexcept {
        const auto n=buffer[ch].size();
        delay=std::clamp(delay,0.,static_cast<double>(n-2));
        const auto i=static_cast<std::size_t>(delay); const double frac=delay-i;
        const auto a=(pos+n-i)%n, b=(a+n-1)%n;
        return (1.-frac)*buffer[ch][a]+frac*buffer[ch][b];
    }
    double fs=48000,alpha=0,voiceCount=4;
    std::size_t pos=0;
    std::array<std::vector<double>,2> buffer;
    std::array<double,maxVoices> phase{};
    FlangeSettings target,current;
};
}
