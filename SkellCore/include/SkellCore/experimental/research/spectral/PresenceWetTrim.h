#pragma once
#include "IndependentEngines.h"

namespace skellcore::experimental::spectral {
// Opt-in, prepare-time wet gain for the existing presence engine. No added delay,
// allocation, or change to PresenceEngine. A fixed gain is not a peak limiter.
class PresenceWetTrim {
    PresenceEngine engine;
    double wet=1.0, gain=1.0;
public:
    void prepare(double sampleRate, double cutoffHz=2600, double boostDb=3,
                 double thresholdDb=-22, double amount=.7, double wetMix=1,
                 double wetTrimDb=0) noexcept {
        wet=bound(wetMix,0,1,1);
        gain=fromDb(bound(wetTrimDb,-18,18,0));
        engine.prepare(sampleRate,cutoffHz,boostDb,thresholdDb,amount,gain==1.0?wet:1.0);
    }
    void reset() noexcept { engine.reset(); }
    Frame process(Frame input) noexcept {
        auto result=engine.process(input);
        if(gain==1.0)return result;
        input={finite(input.l),finite(input.r)};
        return {input.l*(1-wet)+wet*gain*result.l,input.r*(1-wet)+wet*gain*result.r};
    }
};
} // namespace skellcore::experimental::spectral
