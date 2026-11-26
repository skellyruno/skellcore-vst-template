#pragma once
// Shared independent stereo return stage. Ducking is optional, not implied by a send.
// Configure/prepare off the audio thread; process only pre-existing samples.
#include <algorithm>
#include <cmath>
#include <array>
namespace skellcore::vocal {
struct WetReturnSettings {
    double thresholdDb=-24, ratio=2, attackMs=10, releaseMs=180, rangeDb=12, returnDb=0;
    bool duck=false;
};
class WetReturn {
public:
    bool prepare(double rate,const WetReturnSettings& s) noexcept {
        if(!std::isfinite(rate)||rate<8000||rate>192000||!valid(s))return false;
        settings=s;sr=rate;
        attack=std::exp(-1/(sr*s.attackMs*.001));release=std::exp(-1/(sr*s.releaseMs*.001));
        level=std::pow(10.,s.returnDb/20);reset();return true;
    }
    void reset() noexcept { gainDb=0; }
    static bool valid(const WetReturnSettings& s) noexcept {
        auto in=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
        return in(s.thresholdDb,-90,0)&&in(s.ratio,1,20)&&in(s.attackMs,.1,500)&&in(s.releaseMs,5,5000)&&in(s.rangeDb,0,36)&&in(s.returnDb,-60,12);
    }
    // Key is independent from the return input. Wet-only, no dry feed or feedback.
    std::array<float,2> process(float wetL,float wetR,float keyL,float keyR) noexcept {
        if(!std::isfinite(wetL))wetL=0;
        if(!std::isfinite(wetR))wetR=0;
        if(!std::isfinite(keyL))keyL=0;
        if(!std::isfinite(keyR))keyR=0;
        if(settings.duck){
            const double detector=20*std::log10(std::max(1e-12,double(std::max(std::abs(keyL),std::abs(keyR)))));
            const double target=-std::min(settings.rangeDb,std::max(0.,detector-settings.thresholdDb)*(1-1/settings.ratio));
            const double a=target<gainDb?attack:release;gainDb=a*gainDb+(1-a)*target;
            if(std::abs(gainDb)<1e-12)gainDb=0;
        } else gainDb=0;
        const double g=level*std::pow(10.,gainDb/20);
        return {float(std::clamp(double(wetL),-16.,16.)*g),float(std::clamp(double(wetR),-16.,16.)*g)};
    }
    double reductionDb() const noexcept {return -gainDb;}
private:
    WetReturnSettings settings{};double sr=48000,attack=0,release=0,level=1,gainDb=0;
};
} // namespace skellcore::vocal
