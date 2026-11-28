#pragma once
// Original Skell Sound Match dual-power gate, independently developed from
// paired owner audio. Native research port; no vendor implementation.
#include "OriginalDynamics.h"
namespace skellcore::experimental::dynamics {
class DualDetectorGate {
    double aFast,aSlow,thresholdPower,exponent,makeup;
    double fast=0.,slow=0.;
    static constexpr double minimumGain=0.00003162277660168379; // -90 dB
public:
    DualDetectorGate(double fs,double attack=2.,double release=60.,double thresholdDb=-45.,double expansion=4.,double gain=1.) {
        validRate(fs);
        if(!std::isfinite(attack)||attack<.1||attack>30||!std::isfinite(release)||release<5||release>350||attack>release||
           !std::isfinite(thresholdDb)||thresholdDb< -85||thresholdDb> -15||!std::isfinite(expansion)||expansion<0||expansion>12||
           !std::isfinite(gain)||gain<.05||gain>8)throw std::invalid_argument("gate settings");
        aFast=-std::expm1(-1000./(fs*attack));aSlow=-std::expm1(-1000./(fs*release));
        thresholdPower=std::pow(10.,thresholdDb/10.);exponent=.5*expansion;makeup=gain;
    }
    void reset() noexcept {fast=slow=0.;}
    Frame process(Frame x) noexcept {
        for(auto& v:x)v=clean(v);
        const double p=peak(x),power=p*p;
        fast+=aFast*(power-fast);slow+=aSlow*(power-slow);
        if(fast<1e-24)fast=0.;
        if(slow<1e-24)slow=0.;
        const double env=std::max(fast,slow);
        // Gain is unity above threshold. Direct power-domain expansion is
        // mathematically equivalent to our earlier logarithmic expression.
        double g=1.;
        if(exponent>0.&&env<thresholdPower)g=std::max(minimumGain,std::pow(env/thresholdPower,exponent));
        for(auto& v:x)v*=g*makeup;
        return x;
    }
};
}
