#pragma once
// Original Skell research design, NOT a recovered proprietary DSP implementation.
// Real-time safe process: no heap allocations, locks, or file I/O.
#include "IndependentEngines.h"
#include <array>
#include <cmath>

namespace skellcore::experimental::spectral {

class Spectral4 {
    Biquad lpf[2][3][2]{};
    std::array<double,4> env{}, gain{}, base{};
    double wet = 1.0, threshold = 0.2, depth = 0.0, outputTrim = 1.0;
    double rise = 0.0, fall = 0.0, gainPole = 0.0;
public:
    // hz are monotonically increasing crossover frequencies (not vendor settings).
    void prepare(double fsInput, std::array<double,3> crossovers = {230,2400,7500},
                 std::array<double,4> bandDb = {0,0,0,0},
                 double thresholdDb = -18, double depthInput = 0,
                 double wetInput = 1, double outputDb = 0) noexcept {
        const double fs=bound(fsInput,8000,384000,48000);
        double prev=35;
        for(int k=0;k<3;++k) {
            const double lo=prev+30, hi=fs*0.45-30*(2-k);
            const double f=std::clamp(std::isfinite(crossovers[k])?crossovers[k]:prev+500,lo,hi);
            prev=f;
            for(int channel=0;channel<2;++channel)
                for(int stage=0;stage<2;++stage)
                    lpf[channel][k][stage].setButterworth(fs,f,false);
        }
        for(int k=0;k<4;++k) base[k]=fromDb(bound(bandDb[k],-18,18,0));
        threshold=fromDb(bound(thresholdDb,-72,0,-18));
        depth=bound(depthInput,0,3,0);
        wet=bound(wetInput,0,1,1);
        outputTrim=fromDb(bound(outputDb,-18,6,0));
        rise=pole(2,fs);fall=pole(85,fs);gainPole=pole(3.0,fs);
        reset();
    }
    void reset() noexcept {
        for(auto &channel:lpf) for(auto &split:channel) for(auto &section:split)section.reset();
        env.fill(0.0);gain=base;
    }
    Frame process(Frame x) noexcept {
        const double input[2]={finite(x.l),finite(x.r)};
        double bands[2][4];
        for(int channel=0;channel<2;++channel){
            double remainder=input[channel];
            for(int k=0;k<3;++k){
                const double low=lpf[channel][k][1].process(lpf[channel][k][0].process(remainder));
                bands[channel][k]=low;
                remainder-=low;
            }
            bands[channel][3]=remainder;
        }
        double result[2]={0.0,0.0};
        for(int k=0;k<4;++k){
            const double peak=std::max(std::abs(bands[0][k]),std::abs(bands[1][k]));
            const double p=peak>env[k]?rise:fall;
            env[k]=p*env[k]+(1.0-p)*peak;
            const double excess=std::max(0.0,env[k]/threshold-1.0);
            const double target=base[k]/(1.0+depth*excess);
            gain[k]=gainPole*gain[k]+(1.0-gainPole)*target;
            for(int channel=0;channel<2;++channel)result[channel]+=bands[channel][k]*gain[k];
        }
        return {(input[0]*(1.0-wet)+result[0]*wet)*outputTrim,
                (input[1]*(1.0-wet)+result[1]*wet)*outputTrim};
    }
};
} // namespace skellcore::experimental::spectral
